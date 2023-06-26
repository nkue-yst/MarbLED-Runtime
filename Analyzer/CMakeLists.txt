cmake_minimum_required(VERSION 3.23)
project(touchmatrix_analyzer)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)
find_package(cppzmq REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} include)

add_executable(analyzer analyzer.cpp)
target_link_libraries(analyzer ${OpenCV_LIBRARIES} cppzmq)

