# Serial library CMake configuration
cmake_minimum_required(VERSION 3.16.0)

add_compile_options(-std=c++17)

set(target "raspi_sound")

project(${target})

set(include_path
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "/opt/homebrew/include"
)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(sources
  ${source_path}/usbMicro.cpp
)

add_library(${target} ${sources})

target_include_directories(${target}
  PUBLIC
  ${include_path}
)

