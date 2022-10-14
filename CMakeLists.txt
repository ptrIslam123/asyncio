cmake_minimum_required(VERSION 3.23)
project(asyncio)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(examples)
add_subdirectory(asyncio_driver)
add_subdirectory(thread_pool)
add_subdirectory(future)
add_subdirectory(io_event)

add_executable(asyncio main.cpp)

target_link_libraries(${PROJECT_NAME} INTERFACE
    future
    io_event_driver
)
target_link_libraries(${PROJECT_NAME} PUBLIC static_thread_pool asyncio_driver)