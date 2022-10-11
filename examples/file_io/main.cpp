#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include <functional>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "io_event/io_event_poll.h"

const auto readFromFd = [](const auto fd){
    std::array<char, 256> buffer = {0};
    const size_t size = read(fd, buffer.data(), buffer.size());
    std::cout << "read data from fifo: " << buffer.data() << std::endl;
    return size;
};

const auto writeToFd = [](const auto fd, const std::string &data){
    std::array<char, 256> buffer = {0};
    std::copy(data.cbegin(), data.cend(), buffer.begin());
    const auto size = write(fd, buffer.data(), buffer.size());
    std::cout << "write dat to fifo: " << buffer.data() << std::endl;
    return size;
};

int main() {
    const auto handlerForRead = [](const int fd){
        if (readFromFd(fd) <= 0) {
            return io_event::DescriptorStatus::Close;
        }
        return io_event::DescriptorStatus::Open;
    };

    const auto handlerForWrite = [](const int fd){
        if (writeToFd(fd, "Hello world!\n") <= 0) {
            return io_event::DescriptorStatus::Close;
        }
        return io_event::DescriptorStatus::Open;
    };
    io_event::MultiThreadIOEventPoll<std::function<io_event::DescriptorStatus(int)>> ioPoll(10);
    const auto path("tmp/file");
    const auto fdForWrite = open(path, O_WRONLY);
    const auto fdForRead = open(path, O_RDONLY);
    if (fdForRead < 0 || fdForWrite < 0) {
        std::cerr << "Can`t open fifo fd for read or write" << std::endl;
        return EXIT_FAILURE;
    }

    std::thread th([&ioPoll](){
        ioPoll.eventLoop();
    });

    ioPoll.subscribe(fdForWrite, io_event::Event::Write, handlerForWrite);
    ioPoll.subscribe(fdForRead, io_event::Event::Read, handlerForRead);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ioPoll.stopEventLoop();
    th.join();
    return EXIT_SUCCESS;
}
