#ifndef ASYNCIO_SHARED_STATE_H
#define ASYNCIO_SHARED_STATE_H

#include <optional>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace future {

namespace details {

template<typename T>
class SharedState {
public:
    using Value = T;

    SharedState();

    bool isReady() const;

    void setValue(const Value &value);

    void setValue(Value &&value);

    Value getValueIfReadyOrWait();

private:
    using SharedValue = std::optional<Value>;
    SharedValue value_;
    std::mutex valueLock_;
    std::condition_variable eventCheckerThatValueIsReady_;
    std::atomic<bool> isReady_;
};

template<typename T>
SharedState<T>::SharedState():
value_(),
valueLock_(),
eventCheckerThatValueIsReady_(),
isReady_(false) {
}

template<typename T>
bool SharedState<T>::isReady() const {
    return isReady_.load();
}

template<typename T>
void SharedState<T>::setValue(const Value &value) {
    std::unique_lock lock(valueLock_);
    if (isReady()) {
        lock.unlock();
        throw std::runtime_error("Attempt set value more than once");
    }

    value_ = std::move(std::optional(value));
    isReady_.store(true);
    eventCheckerThatValueIsReady_.notify_one();
    lock.unlock();
}

template<typename T>
void SharedState<T>::setValue(Value &&value) {
    std::unique_lock lock(valueLock_);
    if (isReady()) {
        throw std::runtime_error("Attempt set value more than once");
    }

    value_ = std::move(std::optional(std::move(value)));
    isReady_.store(true);
    eventCheckerThatValueIsReady_.notify_one();
    lock.unlock();
}

template<typename T>
typename SharedState<T>::Value SharedState<T>::getValueIfReadyOrWait() {
    std::unique_lock<std::mutex> lock(valueLock_);
    if (!isReady()) {
        eventCheckerThatValueIsReady_.wait(lock, [this]() {
            return isReady();
        });
    }

    auto tmpValue(std::move(value_.value()));
    lock.unlock();
    return std::move(tmpValue);
}

} // namespace details

} // namespace future

#endif //ASYNCIO_SHARED_STATE_H
