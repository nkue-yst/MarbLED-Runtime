cmake_minimum_required(VERSION 3.22)
project(touchmatrix_driver)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

add_executable(driver driver.cpp serial.cpp board.cpp eth.cpp)

# add_compile_definitions(driver NO_BOARD)

target_link_libraries(driver cppzmq udev)
if (!NO_BOARD)
    target_link_libraries(driver udev)
endif (!NO_BOARD)

