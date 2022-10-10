#include "static_thread_pool.h"

#include <algorithm>
#include <stdexcept>

namespace thread {

StaticThreadPool::StaticThreadPool(const unsigned short threadsCount):
workers_(threadsCount),
queue_(),
queueEventDriver_(),
queueLock_() {
    std::generate_n(workers_.begin(), threadsCount, [this]() {
        return std::thread([this]() {
            work();
        });
    });
}

void StaticThreadPool::submitTask(const StaticThreadPool::Task &task) {
    std::lock_guard lock(queueLock_);
    try {
        queue_.push(task);
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error("Can`t submit a new task to static thread pool. "
                                 "Resource limit exceeded");
    }
    queueEventDriver_.notify_one();
}

void StaticThreadPool::join() {
    for (auto i = 0; i < workers_.size(); ++i) {
        submitTask({});
    }

    std::for_each_n(workers_.begin(), workers_.size(), [](auto &worker){
        if (worker.joinable()) {
            worker.join();
        }
    });
}

void StaticThreadPool::work() {
    while (true) {
        std::unique_lock lock(queueLock_);
        while (queue_.empty()) {
            queueEventDriver_.wait(lock);
        }

        auto task = queue_.back();
        queue_.pop();
        lock.unlock();

        if (!task) {
            break;
        } else {
            task();
        }
    }
}

} // namespace thread