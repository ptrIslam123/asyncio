#ifndef ASYNCIO_ASYNCIO_DRIVER_H
#define ASYNCIO_ASYNCIO_DRIVER_H

#include <functional>
#include <memory>

#include "static_thread_pool.h"
#include "future/future.h"
#include "io_event/io_event_driver.h"

namespace asyncio {

class AsyncIODriver {
public:
    using Descriptor = int;
    using AsyncIOEventHandler = std::function<io_event::DescriptorStatus(Descriptor)>;
    using IOEventDriver = io_event::IOEventDriver<AsyncIOEventHandler>;
    template<typename T>
    using AsyncIOHandler = std::function<T(Descriptor)>;
    template<typename T>
    using AsyncIOPackageTask = future::PackageTask<std::function<T(Descriptor)>, Descriptor>;
    template<typename T>
    using AsyncIOResult = typename AsyncIOPackageTask<T>::FutureType;

    AsyncIODriver(std::unique_ptr<IOEventDriver> &&ioEventDriver, short threadsCount);
    ~AsyncIODriver();

    template<typename T>
    AsyncIOResult<T> read(int fd, AsyncIOHandler<T> asyncIoHandler);

    template<typename T>
    void write(int fd, AsyncIOHandler<T> asyncIoHandler);

private:
    std::unique_ptr<IOEventDriver> ioEventDriver_;
    thread::StaticThreadPool staticThreadPool_;
};

template<typename T>
AsyncIODriver::AsyncIOResult<T> AsyncIODriver::read(const Descriptor fd, AsyncIOHandler<T> asyncIoHandler) {
    future::Promise<T> promise;
    auto future = promise.getFuture();
    const auto asyncCallback = [promise, asyncIoHandler, this](const auto fd) mutable {
        staticThreadPool_.submitTask([fd, promise, asyncIoHandler]() mutable {
            promise.setValue(asyncIoHandler(fd));
        });
        return io_event::DescriptorStatus::Close;
    };
    ioEventDriver_->subscribe(fd, io_event::Event::Read, asyncCallback);
    return future;
}

template<typename T>
void AsyncIODriver::write(const Descriptor fd, AsyncIODriver::AsyncIOHandler<T> asyncIoHandler) {
    const auto asyncCallback = [this, asyncIoHandler](const auto fd) mutable {
        staticThreadPool_.submitTask([fd, asyncIoHandler](){
            asyncIoHandler(fd);
        });
        return io_event::DescriptorStatus::Close;
    };
    ioEventDriver_->subscribe(fd, io_event::Event::Write, asyncCallback);
}

} // namespace asyncio

#endif //ASYNCIO_ASYNCIO_DRIVER_H
