cmake_minimum_required(VERSION 3.16)
project(armor_detector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

add_executable(armor_detector main.cpp)
target_link_libraries(armor_detector PRIVATE ${OpenCV_LIBS})
