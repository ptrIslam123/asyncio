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

    int timeout = 10000;
    io_event::MultiThreadIOEventPoll<std::function<io_event::DescriptorStatus(int)>> ioPoll(10, timeout);
    const auto fd = open("tmp/file", O_RDONLY);
    if (fd < 0) {
        std::cerr << "Can`t open file" << std::endl;
        return EXIT_FAILURE;
    }

    std::thread th([&ioPoll]{
        ioPoll.eventLoop();
    });

    ioPoll.subscribe(fd, io_event::Event::Read, [](const int fd) {
        static auto counter = 0;
        if (++counter > 10) {
            return io_event::DescriptorStatus::Close;
        }
        readFromFd(fd);
        return io_event::DescriptorStatus::Open;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ioPoll.stopEventLoop();
    th.join();
    return EXIT_SUCCESS;
}
