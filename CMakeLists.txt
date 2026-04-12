cmake_minimum_required(VERSION 3.20)

project(NullexC C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compiler flags

if (MSVC OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Oi /Ob3 /fp:fast /GL /arch:AVX2)
        add_compile_definitions(USE_BMI2)
        add_link_options(/LTCG)
    endif()
else()
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O3 -march=native -flto -funroll-loops)
        add_compile_definitions(USE_BMI2)
        add_link_options(-flto)
    endif()
endif()

include_directories(.)

# Source discovery

file(GLOB_RECURSE ENGINE_SOURCES
    data/*.c
    functions/*.c
    uci/*.c
)

set(MAIN_SOURCE main.c)

# Main executable

add_executable(NullexC
    ${MAIN_SOURCE}
    ${ENGINE_SOURCES}
)