cmake_minimum_required(VERSION 3.23)
project(touchmatrix_storage)

set(CMAKE_CXX_STANDARD 14)

find_package(cppzmq REQUIRED)
find_package(Threads REQUIRED)

add_executable(storage storage.cpp)
target_link_libraries(storage sqlite3 cppzmq)

