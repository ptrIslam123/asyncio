#ifndef ASYNCIO_FUTURE_H
#define ASYNCIO_FUTURE_H

#include <utility>
#include <thread>
#include "promise.h"

namespace future {

enum class ExecPolicy {
    Async,
    Sync
};

template<typename Functor, typename ... Args>
class PackageTask {
public:
    using ReturnType = decltype(Functor()(Args()...));
    using PromiseType = Promise<ReturnType>;
    using FutureType = typename PromiseType::Future;

    explicit PackageTask(const Functor &functor, const Args& ...args);
    FutureType getFuture();

private:
    FutureType future_;
};

template<typename Functor, typename... Args>
PackageTask<Functor, Args...>::PackageTask(const Functor &functor, const Args& ...args):
future_() {
    PromiseType promise;
    future_ = std::move(promise.getFuture());
    std::thread(std::move(functor), args ... ).detach();
}

template<typename Functor, typename... Args>
typename PackageTask<Functor, Args...>::FutureType PackageTask<Functor, Args...>::getFuture() {
    return std::move(future_);
}

template<typename Functor, typename ... Args>
auto DoPromise(const ExecPolicy policy, Functor &&functor, Args&& ...args) {
    using ReturnType = decltype(Functor()(Args()...));
    Promise<ReturnType> promise;
    switch (policy) {
        case ExecPolicy::Sync: {
            promise.setValue(functor(
                std::forward<Args>(args) ...
            ));
            return promise.getFuture();
        }
        case ExecPolicy::Async: {
            PackageTask<Functor, Args ...> packageTask(functor, args ...);
            return packageTask.getFuture();
        }
        default: {
            return Promise<ReturnType>().getFuture();
        }
    }
}

} // namespace future

#endif //ASYNCIO_FUTURE_H
