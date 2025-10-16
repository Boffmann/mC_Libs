cmake_minimum_required(VERSION 3.16.0)

add_compile_options(-std=c++17)

set(target "pushButton")

project(${target})

set(include_path
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(sources
  ${source_path}/pushButton.cpp
)

add_library(${target} ${sources})

target_include_directories(${target}
  PRIVATE
  ${include_path}
)
