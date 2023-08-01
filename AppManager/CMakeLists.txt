### Setup cmake project ###
cmake_minimum_required(VERSION 3.10)

### Setup C++ project ###
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-fPIC -g -O3 -pthread")

### Setup thirdparty libraries ###
find_package(OpenCV REQUIRED)
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/thirdparty/TUIO11_CPP/oscpack
    ${CMAKE_SOURCE_DIR}/thirdparty/TUIO11_CPP/TUIO
    ${CMAKE_SOURCE_DIR}/thirdparty/libzmq/include
    ${CMAKE_SOURCE_DIR}/thirdparty/cppzmq
)

### Create TouchLED-Library (main project) ###
file(GLOB TLL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
include_directories(include)
add_library(TLL_LIB SHARED ${TLL_SRC})
target_link_libraries(TLL_LIB ${OpenCV_LIBRARIES} cppzmq oscpack TUIO)

### Setup base application ###
option(TLL_APP_MANAGER "Build app manager" ON)
if(TLL_APP_MANAGER)
    add_executable(TLL_AppManager ${CMAKE_CURRENT_SOURCE_DIR}/src/AppManager.cpp)
    target_link_libraries(TLL_AppManager ${CMAKE_DL_LIBS} TLL_LIB TUIO stdc++fs)
endif()
