cmake_minimum_required(VERSION 3.20)

project(future)

add_library(${PROJECT_NAME} INTERFACE
    shared_state.h
    promise.h
    future.h
)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})