cmake_minimum_required(VERSION 3.22)
project(touchmatrix_driver)

set(CMAKE_CXX_STANDARD 14)

find_package(cppzmq REQUIRED)
find_package(Threads REQUIRED)

add_executable(driver driver.cpp serial.cpp)

add_compile_definitions(driver NO_BOARD)

target_link_libraries(driver cppzmq)
if (!NO_BOARD)
    target_link_libraries(driver udev)
endif (!NO_BOARD)

