cmake_minimum_required(VERSION 3.15)

include(scripts/config.cmake)

set(KXEMU_TARGET kxemu-system-${ISA})
set(EXPORT_TARGET libkxemu-${ISA})

project(kxemu-system VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(
    -Wall -Wextra -Werror
    -Wno-unused-command-line-argument -Wno-unused-parameter -Wno-unused-private-field -Wno-gnu-anonymous-struct -Wno-nested-anon-types
    -Ofast -O3
)

include(${CMAKE_SOURCE_DIR}/scripts/kxemu.cmake)

include(${CMAKE_SOURCE_DIR}/scripts/isa/${BASE_ISA}.cmake)
