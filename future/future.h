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
            auto future = promise.getFuture();
            std::thread([promise = std::move(promise), functor = std::forward<Functor>(functor)]
            (auto&& ...args) mutable {
                promise.setValue(functor(
                    std::forward<Args>(args) ...
                ));
            }, std::forward<Args>(args) ...).detach();
            return future;
        }
        default: {
            return Promise<ReturnType>().getFuture();
        }
    }
}

} // namespace future

#endif //ASYNCIO_FUTURE_H
