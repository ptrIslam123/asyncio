#ifndef ASYNCIO_IO_EVENT_POLL_H
#define ASYNCIO_IO_EVENT_POLL_H

#include <poll.h>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <atomic>

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

    explicit IOEventPoll(size_t size);
    void subscribe(int fd, Event event, const Callback &callback) override;
    void unsubscribe(int fd) override;
    void eventLoop() override;
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
IOEventPoll<Functor, Mutex>::IOEventPoll(const size_t size):
IOEventDriver<Functor>(),
fdSet_(),
sessions_(),
sessionsLock_(),
isStoped_(false) {
    fdSet_.reserve(size);
    sessions_.reserve(size);
}

template<typename Functor, typename Mutex>
int IOEventPoll<Functor, Mutex>::getReadyFdCount() {
    const auto count = poll(fdSet_.data(), fdSet_.size(), -1);
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
            callback(curPollFd.fd);
            --readyFdCount;
        }
    }
}

template<typename Functor, typename Mutex>
void IOEventPoll<Functor, Mutex>::stopEventLoop() {
    isStoped_.store(true);
}

} // namespace io_event

#endif //ASYNCIO_IO_EVENT_POLL_H
