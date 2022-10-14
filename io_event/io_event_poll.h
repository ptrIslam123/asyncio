#ifndef ASYNCIO_IO_EVENT_POLL_H
#define ASYNCIO_IO_EVENT_POLL_H

#include <poll.h>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <atomic>
#include <sstream>
#include <unistd.h>

#include "io_event_driver.h"

namespace io_event {

struct MutexMoc;

template<typename Functor, typename Mutex>
class IOEventPoll;

template<typename Functor>
using SingleThreadIOEventPoll = IOEventPoll<Functor, MutexMoc>;

template<typename Functor, typename Mutex = std::mutex>
using MultiThreadIOEventPoll = IOEventPoll<Functor, MutexMoc>;


struct MutexMoc {
    void lock() {}
    void unlock() {}
};

template<typename Functor, typename Mutex>
class IOEventPoll final : public IOEventDriver<Functor> {
public:
    using FdSet = std::vector<pollfd>;
    using Callback = typename IOEventDriver<Functor>::Callback;
    using Timeout = typename IOEventDriver<Functor>::Timeout;

    explicit IOEventPoll(size_t size, int timeout = -1);
    ~IOEventPoll();
    void subscribe(int fd, Event event, const Callback &callback) override;
    void unsubscribe(int fd) override;
    void eventLoop() override;
    void setTimeout(const Timeout &timeout) override;
    void stopEventLoop() override;

private:
    using Session = std::pair<pollfd, Callback>;
    using Sessions = std::vector<Session>;

    int getReadyFdCount();
    void handleReadyFd(size_t readyFdCount);

    FdSet fdSet_;
    Sessions sessions_;
    Mutex sessionsLock_;
    std::atomic<bool> isStoped_;
    int timeout_;
};

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::subscribe(
        const int fd, const Event event, const Callback &callback) {
    struct pollfd pollFd = {.fd = fd};
    switch (event) {
        case Event::Read: {
            pollFd.events = POLLRDNORM;
            break;
        }
        case Event::Write: {
            pollFd.events = POLLWRNORM;
            break;
        }
        default: {
        }
    }

    std::lock_guard lock(sessionsLock_);
    fdSet_.push_back(pollFd);
    sessions_.push_back(std::make_pair(pollFd, callback));
}

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::unsubscribe(const int fd) {
    std::lock_guard lock(sessionsLock_);
    std::erase_if(fdSet_, [fd](const auto &pollFdItem){
        return pollFdItem.fd == fd;
    });
    std::erase_if(sessions_, [fd](const auto &sessionItem){
        return sessionItem.first.fd == fd;
    });
}

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::eventLoop() {
    while (!isStoped_.load()) {
        handleReadyFd(getReadyFdCount());
    }
}

template<typename Functor, typename Mutex>
IOEventPoll<Functor, Mutex>::IOEventPoll(const size_t size, const int timeout):
IOEventDriver<Functor>(),
fdSet_(),
sessions_(),
sessionsLock_(),
isStoped_(false),
timeout_(timeout) {
    fdSet_.reserve(size);
    sessions_.reserve(size);
}

template<typename Functor, typename Mutex>
int IOEventPoll<Functor, Mutex>::getReadyFdCount() {
    const auto count = poll(fdSet_.data(), fdSet_.size(), timeout_);
    if (count < 0) {
        throw std::runtime_error("system call poll is failed");
    }
    return count;
}

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::handleReadyFd(size_t readyFdCount) {
    const auto size = fdSet_.size();
    for (auto i = 0; readyFdCount > 0 && i < size; ++i) {
        const auto &curPollFd = fdSet_[i];
        const auto &curSession = sessions_[i];
        auto callback = curSession.second;
        if (curPollFd.revents == curSession.first.events) {
            if (callback(curPollFd.fd) == DescriptorStatus::Close) {
                unsubscribe(curPollFd.fd);
            }
            --readyFdCount;
        }
    }
}

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::stopEventLoop() {
    isStoped_.store(true);
}

template<typename Functor, typename Mutex>
IOEventPoll<Functor, Mutex>::~IOEventPoll() {
    std::for_each(fdSet_.cbegin(), fdSet_.cend(), [](const auto &pollFdItem){
        if (close(pollFdItem.fd) < 0) {
            std::stringstream os;
            os << "Can`t close file descriptor with number: " << pollFdItem.fd;
            throw std::runtime_error(os.str());
        }
    });
}

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::setTimeout(const IOEventPoll::Timeout &timeout) {
    timeout_ = timeout.count();
}

} // namespace io_event

#endif //ASYNCIO_IO_EVENT_POLL_H
