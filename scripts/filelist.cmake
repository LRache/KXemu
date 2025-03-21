file(
    GLOB_RECURSE EMU_SRCS
    ${CMAKE_SOURCE_DIR}/src/cpu/${BASE_ISA}/**.cpp
    ${CMAKE_SOURCE_DIR}/src/device/**.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/**.cpp
)
list(APPEND EMU_SRCS ${CMAKE_SOURCE_DIR}/src/log.cpp)

file(
    GLOB_RECURSE KDB_SRCS
    ${CMAKE_SOURCE_DIR}/src/kdb/**.cpp
    ${CMAKE_SOURCE_DIR}/src/isa/${BASE_ISA}/**.cpp
)
list(APPEND KDB_SRCS ${CMAKE_SOURCE_DIR}/src/main.cpp)
