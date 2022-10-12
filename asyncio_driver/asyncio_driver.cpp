#include "asyncio_driver.h"

namespace asyncio {

AsyncIODriver::AsyncIODriver(std::unique_ptr<IOEventDriver> &&ioEventDriver,
                             const short threadsCount):
ioEventDriver_(std::move(ioEventDriver)),
staticThreadPool_(static_cast<unsigned short>(std::clamp(threadsCount - 1, 1, 256))) {
    std::thread([this]{
        ioEventDriver_->eventLoop();
    }).detach();
    /// Так не работает, так как процесс опроса дескрипторов в ядре и процессы регитсрации новых
    /// отслеживаемых дескрипторов это разные потоки. Нужно что бы глав поток опроса дескрипторов
    /// Время от времени возвращался в user space что бы побавить новые дескрипторы в набор
    /// отслеживаемых!
}

AsyncIODriver::~AsyncIODriver() {
    ioEventDriver_->stopEventLoop();
    staticThreadPool_.join();
}

} // namespace asyncio