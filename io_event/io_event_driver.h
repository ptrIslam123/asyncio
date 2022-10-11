#ifndef ASYNCIO_IO_EVENT_DRIVER_H
#define ASYNCIO_IO_EVENT_DRIVER_H

namespace io_event {

enum class Event {
    Read,
    Write
};

enum class DescriptorStatus {
    Open,
    Close
};

template<typename Functor>
class IOEventDriver {
public:
    using Callback = Functor;

    virtual void subscribe(int fd, Event event, const Callback &callback) = 0;
    virtual void unsubscribe(int fd) = 0;
    virtual void eventLoop() = 0;
    virtual void stopEventLoop() = 0;
    virtual ~IOEventDriver() = default;
};

} // namespace io_event

#endif //ASYNCIO_IO_EVENT_DRIVER_H
