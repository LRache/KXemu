cmake_minimum_required(VERSION 3.15)

option(TARGET_APP "Build Executable" ON)
option(TARGET_LIB "Build Libarary" OFF)

include(scripts/config.cmake)

set(KXEMU_TARGET kxemu-system-${ISA})
set(EXPORT_TARGET libkxemu-${ISA})

project(kxemu-system VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(
    -Wall -Wextra -Werror
    -Wno-unused-command-line-argument -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-nested-anon-types
    -Ofast -O3
)

set(KXEMU_TARGET kxemu-system-${ISA})
set(LIB_TARGET kxemu-${ISA})

include_directories(${CMAKE_SOURCE_DIR}/include)

include(${CMAKE_SOURCE_DIR}/scripts/filelist.cmake)

if (TARGET_APP)
    include(${CMAKE_SOURCE_DIR}/scripts/kxemu.cmake)
endif()

if (TARGET_LIB)
    include(${CMAKE_SOURCE_DIR}/scripts/libkxemu.cmake)
endif()

include(${CMAKE_SOURCE_DIR}/scripts/isa/${BASE_ISA}.cmake)
