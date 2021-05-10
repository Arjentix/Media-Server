cmake_minimum_required(VERSION 3.13)
project(media-server)

set(SRC_DIR src)

set(BUILD_FLAGS "-Wall -Wextra -pedantic")

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/split.cpp
    ${SRC_DIR}/port_handler/port_handler_manager.cpp
    ${SRC_DIR}/frame/provider.cpp
    ${SRC_DIR}/frame/provider_manager.cpp
    ${SRC_DIR}/sock/exception.cpp
    ${SRC_DIR}/sock/socket.cpp
    ${SRC_DIR}/sock/server_socket.cpp
    ${SRC_DIR}/sock/client_socket.cpp
    ${SRC_DIR}/rtsp/client.cpp
    ${SRC_DIR}/rtsp/request.cpp
    ${SRC_DIR}/rtsp/response.cpp
    ${SRC_DIR}/sdp/session_description.cpp
    ${SRC_DIR}/rtp/deserializable.cpp
    ${SRC_DIR}/rtp/packet.cpp
    ${SRC_DIR}/rtp/mjpeg/packet.cpp
    ${SRC_DIR}/converters/mjpeg_to_h264.cpp
    ${SRC_DIR}/converters/mpeg2ts_packager.cpp
    ${SRC_DIR}/http/base_request.cpp
)

find_package(Threads REQUIRED)


# Finding FFMPEG
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec REQUIRED)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat REQUIRED)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil REQUIRED)

find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale REQUIRED)


add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_DIR}
    ${AVCODEC_INCLUDE_DIR}
    ${AVFORMAT_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    ${SWSCALE_INCLUDE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Threads::Threads
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_FLAGS ${BUILD_FLAGS}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
)
