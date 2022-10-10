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

template<typename Functor>
auto DoPromise(Functor &&functor, const ExecPolicy policy = ExecPolicy::Async) {
    Promise<Functor> promise;
    switch (policy) {
        case ExecPolicy::Sync: {
            promise.setValue(functor());
            return promise.getFuture();
        }
        case ExecPolicy::Async: {
            auto future = promise.getFuture();
            std::thread([promise = std::move(promise), functor = std::forward<Functor>(functor)]
            () mutable {
                promise.setValue(functor());
            }).detach();
            return future;
        }
        default: {
            return Promise<Functor>().getFuture();
        }
    }
}

} // namespace future

#endif //ASYNCIO_FUTURE_H
