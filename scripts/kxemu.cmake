add_executable(${KXEMU_TARGET}
    ${EMU_SRCS}
    ${KDB_SRCS}
)

target_compile_options(${KXEMU_TARGET} PRIVATE -flto)
target_link_options(${KXEMU_TARGET} PRIVATE -flto)

# LLVM
find_package(LLVM REQUIRED CONFIG)

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    MC
    Support
    ${BASE_ISA_UPPER}Disassembler
)

include_directories(${LLVM_INCLUDE_DIRS})

# gdbstub
set(GDBSTUB_LIB ${CMAKE_SOURCE_DIR}/utils/mini-gdbstub/build/libgdbstub.a)
add_custom_command(
    OUTPUT  ${GDBSTUB_LIB}
    COMMAND make -C ${CMAKE_SOURCE_DIR}/utils/mini-gdbstub
    COMMENT "BUILD UTIL: mini-gdbstub"
)
add_custom_target(GDBSTUB_LIB DEPENDS ${GDBSTUB_LIB})
include_directories(${CMAKE_SOURCE_DIR}/utils/mini-gdbstub/include)

add_dependencies(${KXEMU_TARGET} GDBSTUB_LIB)

target_link_libraries(${KXEMU_TARGET} ${LLVM_LIBS} ${GDBSTUB_LIB} readline)
