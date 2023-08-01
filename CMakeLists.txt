### Setup cmake project ###
cmake_minimum_required(VERSION 3.10)
project(MarbLED-runtime)

### Build thirdparty libraries ###
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty)

### Copy engine component files (Need to change) ###
file(COPY 
    ${CMAKE_CURRENT_SOURCE_DIR}/NotoSansJP-Regular.otf
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)

### Build main programs ###
add_subdirectory(AppManager)
