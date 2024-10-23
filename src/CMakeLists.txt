cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOMOC ON)

project(QtKcp VERSION 1.0 LANGUAGES C CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)

add_library(QtKcp SHARED
    ${CMAKE_SOURCE_DIR}/3rdparty/kcp/ikcp.c
    ${CMAKE_SOURCE_DIR}/3rdparty/kcp/ikcp.h
    qkcpserver.cpp
    qkcpserver.h
    qkcpsocket.cpp
    qkcpsocket.h
    qkcpsocket_global.h
    qkcpsocket_p.h
)

target_include_directories(QtKcp PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/kcp)

set_target_properties(QtKcp PROPERTIES
    DEBUG_POSTFIX "d"
    PUBLIC_HEADER "qkcpsocket_global.h;qkcpsocket.h;qkcpserver.h"
    PDB_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/buildlib/${CMAKE_BUILD_TYPE}
)

target_compile_options(QtKcp PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/Zi>")

target_compile_definitions(QtKcp PRIVATE QTKCP_LIBRARY)

target_link_libraries(QtKcp PRIVATE Qt::Network)

install(TARGETS QtKcp
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/buildlib/include
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/buildlib/${CMAKE_BUILD_TYPE}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/buildlib/${CMAKE_BUILD_TYPE}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/buildlib/${CMAKE_BUILD_TYPE}
)
