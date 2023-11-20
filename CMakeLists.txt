### Setup cmake project ###
cmake_minimum_required(VERSION 3.22)
project(MarbLED-runtime)

set(CMAKE_CXX_STANDARD 14)

### Build thirdparty libraries ###
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty)

### Copy engine component files (Need to change) ###
file(COPY 
    ${CMAKE_CURRENT_SOURCE_DIR}/NotoSansJP-Regular.otf
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)

include_directories(${CMAKE_SOURCE_DIR}/Utility)

### Build main programs ###
add_subdirectory(AppManager)
add_subdirectory(Storage)
add_subdirectory(Analyzer)
add_subdirectory(Driver)
