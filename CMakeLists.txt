cmake_minimum_required(VERSION 3.13)
project(media-server)

set(SRC_DIR src)

set(BUILD_FLAGS "-Wall -Wextra -pedantic")

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/port_handler/port_handler_manager.cpp
    ${SRC_DIR}/frame/provider.cpp
    ${SRC_DIR}/frame/provider_manager.cpp
    ${SRC_DIR}/sock/exception.cpp
    ${SRC_DIR}/sock/socket.cpp
    ${SRC_DIR}/sock/server_socket.cpp
    ${SRC_DIR}/sock/client_socket.cpp
    ${SRC_DIR}/rtsp/request.cpp
    ${SRC_DIR}/rtsp/response.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_DIR}
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_FLAGS ${BUILD_FLAGS}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
)
