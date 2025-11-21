cmake_minimum_required(VERSION 3.25)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

set(GLM_PATH "C:/dev/glm")

set(SDL3_PATH "C:/dev/SDL3-3.2.26")
include_directories(${SDL3_PATH}/include)
link_directories(${SDL3_PATH}/lib/x64)

include_directories(${GLM_PATH})

add_executable(untitled main.cpp
        circle.h
        circle.cpp)

target_link_libraries(untitled PRIVATE SDL3)