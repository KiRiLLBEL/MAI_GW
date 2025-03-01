cmake_minimum_required(VERSION 3.22.2)

project(dsl-parser)

include(FetchContent)
include(CTest)
enable_testing()

add_library(lang_parser INTERFACE)

target_include_directories(
    lang_parser
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    lang_parser
    INTERFACE
    foonathan::lexy
)

include(FetchContent)
FetchContent_Declare(lexy URL https://lexy.foonathan.net/download/lexy-src.zip)
FetchContent_MakeAvailable(lexy)

add_executable(dsl-parser)
target_sources(
    dsl-parser PRIVATE
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)
target_link_libraries(
    dsl-parser PRIVATE 
    lang_parser
)

add_subdirectory(test)