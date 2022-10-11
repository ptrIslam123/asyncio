cmake_minimum_required(VERSION 3.20)

project(simple_tcp_server)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ../..)
target_link_libraries(${PROJECT_NAME} io_event_driver)