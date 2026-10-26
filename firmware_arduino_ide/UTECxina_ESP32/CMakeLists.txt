cmake_minimum_required(VERSION 4.3)
project(UTECxina_ESP32)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(UTECxina_ESP32
        UTECxina_ESP32.cpp
        UTECxina_ESP32.h)
