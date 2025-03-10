cmake_minimum_required(VERSION 3.22.2)

project(dsl-parser)

include(FetchContent)
include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(
    lexy 
    URL https://lexy.foonathan.net/download/lexy-src.zip
)
FetchContent_Declare(
    magic_enum
    URL https://github.com/Neargye/magic_enum/archive/refs/tags/v0.9.7.zip
)
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.zip
)
FetchContent_MakeAvailable(lexy)
FetchContent_MakeAvailable(magic_enum)
FetchContent_MakeAvailable(fmt)

add_library(lang INTERFACE)

target_include_directories(
    lang
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_link_libraries(
    lang
    INTERFACE
    foonathan::lexy
    magic_enum::magic_enum
    fmt::fmt
)

add_executable(dsl-parser)
target_sources(
    dsl-parser PRIVATE
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)
target_link_libraries(
    dsl-parser PRIVATE 
    lang
)

add_subdirectory(test)