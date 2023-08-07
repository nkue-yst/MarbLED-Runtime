cmake_minimum_required(VERSION 3.22)
project(touchmatrix_storage)

set(CMAKE_CXX_STANDARD 14)

find_package(cppzmq REQUIRED)
find_package(Threads REQUIRED)

add_executable(storage storage.cpp db.cpp)
target_link_libraries(storage sqlite3 cppzmq)

