cmake_minimum_required(VERSION 3.20)

project(asyncio_driver)

add_library(${PROJECT_NAME} asyncio_driver.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ../)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} future io_event_driver)
target_link_libraries(${PROJECT_NAME} static_thread_pool)
