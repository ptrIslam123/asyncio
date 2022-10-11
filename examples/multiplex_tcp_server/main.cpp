#include <iostream>
#include <string>
#include <string_view>
#include <array>
#include <stdexcept>
#include <functional>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <csignal>

#include "io_event/io_event_poll.h"

using IOEventDriver = io_event::MultiThreadIOEventPoll<
        std::function<io_event::DescriptorStatus(const int fd)>>;
constexpr auto maxSessionsCount = 50;
/// Global io event driver
IOEventDriver ioEventDriver(maxSessionsCount);

/**
 * @brief Terminate signal handler. (kill 15 pid)
 * It Close all open file descriptors
 * @param sig signal number (15)
 */
void SigTermHandler(int sig) {
    std::cout << "\t\t\t[Tcp server is stoped]" << std::endl;
    ioEventDriver.stopEventLoop();
    exit(0);
}

/**
 * @brief hang up signal handler. (kill 1)
 * This signal usually use for reconfigure demon (process)
 * @param sig signal number (1)
 */
void SigHupHandler(int sig) {
    std::cout << "\t\t\t[Reconfigure tcp server]" << std::endl;
    /// Do nothing
}

/**
 * @brief Create serve stream(tcp) server socket
 * @param ipAddr Server Ip address
 * @param port Server port number
 * @return Server socket
 */
int GetServerSocket(const std::string_view ipAddr, const unsigned short port) {
    const auto sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw  std::runtime_error("Can`t create socket");
    }

    struct sockaddr_in socketAddr;
    std::memset(&socketAddr, 0, sizeof(socketAddr));
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(port);
    socketAddr.sin_addr.s_addr = inet_addr(ipAddr.data());

    if (bind(sock, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) < 0) {
        throw std::runtime_error("Can`t bind socket address");
    }
    return sock;
}

auto ReadFrom(const int fd) {
    std::array<char, 1024> buffer = {0};
    const auto size = read(fd, buffer.data(), buffer.size());
    return std::make_pair(std::string(buffer.data()), size);
}

auto WriteTo(const int fd, const std::string_view data) {
    auto size = 0;
    while (size < data.size()) {
        const auto sendBuffSize = write(fd, data.data(), data.size());
        if (sendBuffSize < 0) {
            return -1;
        }
        size += static_cast<decltype(size)>(sendBuffSize);
    }
    return size;
}

/**
 * @brief Client io request handler
 * @param fd Client socket (file descriptor)
 */
const auto clientRequestEventHandler = [](const int fd){
    const auto [data, size] = ReadFrom(fd);
    if (size < 0) {
        std::cerr << "read from client socket is failed" << std::endl;
    } else if (size == 0) {
        std::cout << "client connection with socket: " << fd << " is closed" << std::endl;
        return io_event::DescriptorStatus::Close;
    }
    if (WriteTo(fd, data) < 0) {
        std::cerr << "write to client socket: " << fd << " is failed" << std::endl;
    }
    return io_event::DescriptorStatus::Open;
};

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "No passing args: <ip address> <port number>" << std::endl;
        return EXIT_FAILURE;
    }

    /**
     * @brief Register signal handlers [SIGTERM, SIGHUP]
     */
    (void)signal(SIGTERM, SigTermHandler);
    (void) signal(SIGHUP, SigHupHandler);

    const auto serverIpAddress(argv[1]);
    const auto port(std::atoi(argv[2]));
    /// Create serve socket
    const auto serverSock = GetServerSocket(serverIpAddress, port);

    /// Create server socket queue for accepting client connection
    if (listen(serverSock, 10) < 0) {
        std::cerr << "Can`t create connection queue for server socket" << std::endl;
        return EXIT_FAILURE;
    }

    /// Accept event handler
    const auto clientConnectEventHandler = [](const int serverFd){
                const auto clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            std::cerr << "Can`t accept new client connection" << std::endl;
        }
        ioEventDriver.subscribe(clientFd, io_event::Event::Read, clientRequestEventHandler);
        return io_event::DescriptorStatus::Open;
    };

    std::cout << "\t\t\t[Tcp server is running]" << std::endl;
    /// Add accept fd(server socket) to the io event poll
    ioEventDriver.subscribe(serverSock, io_event::Event::Read, clientConnectEventHandler);
    /// Go to the io event loop
    ioEventDriver.eventLoop();
    return 0;
}