#ifndef ASYNCIO_PROMISE_H
#define ASYNCIO_PROMISE_H

#include <memory>
#include "shared_state.h"

namespace future {

template<typename Ret>
class Promise {
public:
    using Value = Ret;
    using SharedState = std::shared_ptr<details::SharedState<Value>>;

    class Future {
    public:
        explicit Future();
        Future(Future &&other)  noexcept;
        Future &operator=(Future &&other) noexcept;
        Future(const Future &other);
        Future &operator=(const Future &other);
        bool isReady() const;
        Value getValue();

    private:
        friend Promise<Value>;
        explicit Future(SharedState sharedState);
        SharedState sharedState_;
    };

    Promise();
    Promise(Promise &&other) noexcept;
    Promise &operator=(Promise &&other) noexcept;
    Promise(const Promise &other);
    Promise &operator=(const Promise &other);
    Future getFuture() const;
    void setValue(const Value &value);
    void setValue(Value &&value);

private:
    SharedState sharedState_;
    mutable unsigned char futureCounter_;
};

template<typename Ret>
Promise<Ret>::Future::Future(SharedState sharedState):
sharedState_(sharedState) {
}

template<typename Ret>
typename Promise<Ret>::Value Promise<Ret>::Future::getValue() {
    if (!sharedState_) {
        throw std::runtime_error("Attempt get access to invalid future value");
    }
    return sharedState_->getValueIfReadyOrWait();
}

template<typename Ret>
bool Promise<Ret>::Future::isReady() const {
    return sharedState_->isReady();
}

template<typename Ret>
typename Promise<Ret>::Future &Promise<Ret>::Future::operator=(Promise::Future &&other) noexcept {
    sharedState_ = other.sharedState_;
    other.sharedState_ = nullptr;
    return *this;
}

template<typename Ret>
Promise<Ret>::Future::Future(Future &&other) noexcept:
sharedState_(other.sharedState_){
    other.sharedState_ = nullptr;
}

template<typename Ret>
Promise<Ret>::Future::Future():
sharedState_(nullptr) {
}

template<typename Ret>
Promise<Ret>::Future::Future(const Promise::Future &other):
sharedState_(other.sharedState_) {
}

template<typename Ret>
typename Promise<Ret>::Future &Promise<Ret>::Future::operator=(const Promise::Future &other) {
    sharedState_ = other.sharedState_;
    return *this;
}

    template<typename Ret>
typename Promise<Ret>::Future Promise<Ret>::getFuture() const {
    ++futureCounter_;
    if (futureCounter_ > 1U) {
        throw std::runtime_error("Attempt get future from promise more than once");
    }
    return Future(sharedState_);
}

template<typename Ret>
void Promise<Ret>::setValue(const Promise::Value &value) {
    sharedState_->setValue(value);
}

template<typename Ret>
void Promise<Ret>::setValue(Promise::Value &&value) {
    sharedState_->setValue(value);
}

template<typename Ret>
Promise<Ret>::Promise():
sharedState_(new details::SharedState<Value>()),
futureCounter_(0U) {
}

template<typename Ret>
Promise<Ret>::Promise(Promise &&other) noexcept:
sharedState_(other.sharedState_){
    other.sharedState_ = nullptr;
}

template<typename Ret>
Promise<Ret> &Promise<Ret>::operator=(Promise &&other) noexcept {
    sharedState_ = other.sharedState_;
    other.sharedState_ = nullptr;
    return *this;
}

template<typename Ret>
Promise<Ret>::Promise(const Promise &other):
sharedState_(other.sharedState_) {
}

template<typename Ret>
Promise<Ret> &Promise<Ret>::operator=(const Promise &other) {
    sharedState_ = other.sharedState_;
    return *this;
}

} // namespace future

#endif //ASYNCIO_PROMISE_H
