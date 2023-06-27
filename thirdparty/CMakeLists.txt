### Setup cmake project ###
cmake_minimum_required(VERSION 3.10)

### Setup C++ project ###
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-fPIC -g -O3 -pthread")

### Setup TUIO ###
file(GLOB TUIO_SRC TUIO11_CPP/TUIO/*.cpp TUIO11_CPP/TUIO/*.h)
add_library(TUIO STATIC ${TUIO_SRC})
include_directories(TUIO11_CPP/TUIO)
target_link_libraries(TUIO oscpack)

### Setup oscpack ###
set(OSCPACK_DIR TUIO11_CPP/oscpack)
include_directories(${OSCPACK_DIR})
if(WIN32)
    set(IpSystemTypePath ip/win32)
else(WIN32)
    set(IpSystemTypePath ip/posix)
endif(WIN32)
add_library(oscpack
    ${OSCPACK_DIR}/ip/IpEndpointName.h
    ${OSCPACK_DIR}/ip/IpEndpointName.cpp
    ${OSCPACK_DIR}/ip/NetworkingUtils.h
    ${OSCPACK_DIR}/${IpSystemTypePath}/NetworkingUtils.cpp
    ${OSCPACK_DIR}/ip/UdpSocket.h
    ${OSCPACK_DIR}/${IpSystemTypePath}/UdpSocket.cpp
    ${OSCPACK_DIR}/ip/PacketListener.h
    ${OSCPACK_DIR}/ip/TimerListener.h
    ${OSCPACK_DIR}/osc/OscTypes.h
    ${OSCPACK_DIR}/osc/OscTypes.cpp 
    ${OSCPACK_DIR}/osc/OscHostEndianness.h
    ${OSCPACK_DIR}/osc/OscException.h
    ${OSCPACK_DIR}/osc/OscPacketListener.h
    ${OSCPACK_DIR}/osc/MessageMappingOscPacketListener.h
    ${OSCPACK_DIR}/osc/OscReceivedElements.h
    ${OSCPACK_DIR}/osc/OscReceivedElements.cpp
    ${OSCPACK_DIR}/osc/OscPrintReceivedElements.h
    ${OSCPACK_DIR}/osc/OscPrintReceivedElements.cpp
    ${OSCPACK_DIR}/osc/OscOutboundPacketStream.h
    ${OSCPACK_DIR}/osc/OscOutboundPacketStream.cpp
)

### Setup ZeroMQ ###
option(WITH_TLS OFF)
option(WITH_LIBSODIUM OFF)
option(BUILD_TESTS OFF)
option(CPPZMQ_BUILD_TESTS OFF)

add_subdirectory(libzmq)
set(ZeroMQ_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/libzmq)
add_subdirectory(cppzmq)

include_directories(libzmq/include)
include_directories(cppzmq)
