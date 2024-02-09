cmake_minimum_required(VERSION 3.22)
project(touchmatrix_analyzer)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)

add_executable(analyzer analyzer.cpp frame.cpp mapper.cpp mapper.h
        tracker.cpp
        tracker.h)
target_link_libraries(analyzer ${OpenCV_LIBRARIES} cppzmq oscpack)

target_include_directories(analyzer PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    include
        ${CMAKE_SOURCE_DIR}/thirdparty/oscpack
)
