cmake_minimum_required(VERSION 3.20)

project(gsexp VERSION 0.1.0 LANGUAGES CXX)

option(GSEXP_STRICT "Enable strict compiler warnings" ON)
option(GSEXP_WARN_AS_ERROR "Treat warnings as errors" OFF)
option(GSEXP_BUILD_EXAMPLES "Build example programs" ON)
option(GSEXP_BUILD_TESTS "Build test programs" ON)
option(GSEXP_BUILD_BENCHMARKS "Build benchmark programs" OFF)
option(GSEXP_BENCHMARK_YYJSON "Fetch yyjson for benchmark comparison" ON)

add_library(gsexp
    src/form_view.cpp
    src/node.cpp
    src/sexp.cpp
    src/tokenize.cpp
)

add_library(gsexp::gsexp ALIAS gsexp)

target_include_directories(gsexp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(gsexp PUBLIC cxx_std_20)

function(gsexp_apply_warnings target_name)
    if(NOT GSEXP_STRICT)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        if(GSEXP_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
        if(GSEXP_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

gsexp_apply_warnings(gsexp)

if(GSEXP_BUILD_EXAMPLES)
    add_executable(gsexp_demo examples/gsexp_demo.cpp)
    target_link_libraries(gsexp_demo PRIVATE gsexp::gsexp)
    target_compile_features(gsexp_demo PRIVATE cxx_std_20)
    gsexp_apply_warnings(gsexp_demo)
endif()

if(GSEXP_BUILD_TESTS)
    enable_testing()

    add_executable(gsexp_tests tests/sexp_tests.cpp)
    target_link_libraries(gsexp_tests PRIVATE gsexp::gsexp)
    target_compile_features(gsexp_tests PRIVATE cxx_std_20)
    gsexp_apply_warnings(gsexp_tests)

    add_test(NAME gsexp_tests COMMAND gsexp_tests)
endif()

if(GSEXP_BUILD_BENCHMARKS)
    if(GSEXP_BENCHMARK_YYJSON)
        enable_language(C)
        include(FetchContent)

        set(YYJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(YYJSON_BUILD_FUZZER OFF CACHE BOOL "" FORCE)
        set(YYJSON_BUILD_MISC OFF CACHE BOOL "" FORCE)
        set(YYJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
        set(YYJSON_INSTALL OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(yyjson
            GIT_REPOSITORY https://github.com/ibireme/yyjson.git
            GIT_TAG 0.12.0
        )
        FetchContent_MakeAvailable(yyjson)
    endif()

    add_executable(gsexp_parse_bench benchmarks/parse_bench.cpp)
    target_link_libraries(gsexp_parse_bench PRIVATE gsexp::gsexp)
    if(GSEXP_BENCHMARK_YYJSON)
        target_link_libraries(gsexp_parse_bench PRIVATE yyjson)
        target_compile_definitions(gsexp_parse_bench PRIVATE GSEXP_HAVE_YYJSON=1)
    endif()
    target_compile_features(gsexp_parse_bench PRIVATE cxx_std_20)
    gsexp_apply_warnings(gsexp_parse_bench)
endif()
