cmake_minimum_required(VERSION 3.16)
project(armor_detector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs geometry videoio)

add_executable(armor_detector main.cpp armor_detector.cpp)
target_link_libraries(armor_detector PRIVATE ${OpenCV_LIBS})

add_executable(armor_video video_main.cpp armor_detector.cpp)
target_link_libraries(armor_video PRIVATE ${OpenCV_LIBS})
