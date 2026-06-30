cmake_minimum_required(VERSION 3.16)

project(deepcircus_bool_bench LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    aiger
    GIT_REPOSITORY https://github.com/arminbiere/aiger.git
    GIT_TAG rel-1.9.20
)
FetchContent_GetProperties(aiger)
if(NOT aiger_POPULATED)
    FetchContent_Populate(aiger)
    add_library(aiger STATIC "${aiger_SOURCE_DIR}/aiger.c")
    target_include_directories(aiger PUBLIC "${aiger_SOURCE_DIR}")
endif()

add_library(bb
    decision_tree.cpp
    decision_tree.h
    bool_bench.cpp
    bool_bench.h
    medium_bitness.cpp
    medium_bitness.h
    small_bitness.cpp
    small_bitness.h
)

target_include_directories(bb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(bb PRIVATE aiger)
