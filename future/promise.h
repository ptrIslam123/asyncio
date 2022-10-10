#ifndef ASYNCIO_PROMISE_H
#define ASYNCIO_PROMISE_H

#include <memory>
#include "shared_state.h"

namespace future {

template<typename Func>
class Promise {
public:
    using Task = Func;
    using Value = std::result_of_t<Task&()>;
    using SharedState = std::shared_ptr<details::SharedState<Value>>;

    class Future {
    public:
        Future(Future &&other)  noexcept;
        Future &operator=(Future &&other) noexcept;
        Future(const Future &other) = delete;
        Future &operator=(const Future &other) = delete;
        bool isReady() const;
        Value getValue();

    private:
        friend Promise<Task>;
        explicit Future(SharedState sharedState);
        SharedState sharedState_;
    };

    Promise();
    Promise(Promise &&other) noexcept;
    Promise &operator=(Promise &&other) noexcept;
    Promise(const Promise &other) = delete;
    Promise &operator=(const Promise &other) = delete;
    Future getFuture() const;
    void setValue(const Value &value);
    void setValue(Value &&value);

private:
    SharedState sharedState_;
    mutable unsigned char futureCounter_;
};

template<typename Func>
Promise<Func>::Future::Future(SharedState sharedState):
sharedState_(sharedState) {
}

template<typename Func>
typename Promise<Func>::Value Promise<Func>::Future::getValue() {
    return sharedState_->getValueIfReadyOrWait();
}

template<typename Func>
bool Promise<Func>::Future::isReady() const {
    return sharedState_->isReady();
}

template<typename Func>
typename Promise<Func>::Future &Promise<Func>::Future::operator=(Promise::Future &&other) noexcept {
    sharedState_ = other.sharedState_;
    other.sharedState_ = nullptr;
}

template<typename Func>
Promise<Func>::Future::Future(Future &&other) noexcept:
sharedState_(other.sharedState_){
    other.sharedState_ = nullptr;
}

template<typename Func>
typename Promise<Func>::Future Promise<Func>::getFuture() const {
    ++futureCounter_;
    if (futureCounter_ > 1U) {
        throw std::runtime_error("Attempt get future from promise more than once");
    }
    return Future(sharedState_);
}

template<typename Func>
void Promise<Func>::setValue(const Promise::Value &value) {
    sharedState_->setValue(value);
}

template<typename Func>
void Promise<Func>::setValue(Promise::Value &&value) {
    sharedState_->setValue(value);
}

template<typename Func>
Promise<Func>::Promise():
sharedState_(new details::SharedState<Value>()),
futureCounter_(0U) {
}

template<typename Func>
Promise<Func>::Promise(Promise &&other) noexcept:
sharedState_(other.sharedState_){
    other.sharedState_ = nullptr;
}

template<typename Func>
Promise<Func> &Promise<Func>::operator=(Promise &&other) noexcept {
    sharedState_ = other.sharedState_;
    other.sharedState_ = nullptr;
    return *this;
}

} // namespace future

#endif //ASYNCIO_PROMISE_H
