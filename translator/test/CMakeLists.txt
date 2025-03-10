cmake_minimum_required(VERSION 3.22.2)
project(parser_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(
    parser_test_smoke
    parser_test_smoke.cpp
)

add_executable(
    translator_test_smoke
    translator_test_smoke.cpp
)

target_link_libraries(
    parser_test_smoke PRIVATE
    gtest
    gtest_main
    lang
)

target_link_libraries(
    translator_test_smoke PRIVATE
    gtest
    gtest_main
    lang
)

add_test(
    NAME ParserTestSmoke
    COMMAND parser_test_smoke
)

add_test(
    NAME TranslatorTestSmoke
    COMMAND translator_test_smoke
)