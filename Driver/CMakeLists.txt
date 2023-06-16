cmake_minimum_required(VERSION 3.23)
project(touchmatrix_driver)

set(CMAKE_CXX_STANDARD 14)

find_package(cppzmq REQUIRED)
find_package(Threads REQUIRED)

add_executable(driver driver.cpp serial.cpp)
target_link_libraries(driver cppzmq udev)

