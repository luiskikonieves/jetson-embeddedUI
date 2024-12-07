cmake_minimum_required(VERSION 3.13)

option(ENABLE_LOGS "Enable testing mode" OFF)

message(STATUS "Test mode: ${ENABLE_LOGS}")

unset(JETSON CACHE)

project(jetson-embeddedUI VERSION 1.0.0

                        DESCRIPTION "Embedded UI hardware control for Jetson Orin Nano."
                        LANGUAGES C CXX)

include(CTest)
enable_testing()

set(SOURCE_FILES
    src/main.cpp
    src/serial.cpp
    src/serial.h
    src/configuration.cpp
    src/configuration.hpp
    src/UiServer.h
    src/UiServer.cpp
    src/WebSystem.h
    src/WebSystem.cpp
    src/ThreadUtils.h
    src/ThreadUtils.cpp
    src/pwm.h
    src/pwm.cpp
    src/IO.h
    src/IO.cpp
)

add_executable(jetson-embeddedUI ${SOURCE_FILES})

if(ENABLE_LOGS)
    target_compile_definitions(jetson-embeddedUI PRIVATE ENABLE_LOGS)
endif()

find_library(WEBSOCKETS_LIBRARY NAMES websockets libwebsockets libwebsockets-dev PATHS /usr/lib /usr/include /usr/lib/aarch64-linux-gnu/ /usr/local/lib)
find_package(nlohmann_json 3.2.0 REQUIRED)

if(NOT WEBSOCKETS_LIBRARY)
    message(FATAL_ERROR "libwebsockets not found")
endif()

if(WEBSOCKETS_LIBRARY)
    message(STATUS "libwebsockets found: ${WEBSOCKETS_LIBRARY}")
    target_link_libraries(jetson-embeddedUI nlohmann_json::nlohmann_json ${WEBSOCKETS_LIBRARY})
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
