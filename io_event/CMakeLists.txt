cmake_minimum_required(VERSION 3.20)

project(io_event_driver)

add_library(${PROJECT_NAME} INTERFACE
    io_event_driver.h
        io_event_poll.h
)