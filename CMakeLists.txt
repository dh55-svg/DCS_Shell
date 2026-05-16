cmake_minimum_required(VERSION 3.20)
project(DCS_Shell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql Network SerialPort Multimedia Test)
find_package(Qt6 QUIET COMPONENTS Mqtt)
set(HAS_QT_MQTT ${Qt6Mqtt_FOUND})

option(BUILD_TESTS "Build tests" ON)
option(BUILD_PLUGINS "Build plugins" ON)

add_subdirectory(plugin_interface)

if(BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

add_subdirectory(host)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
