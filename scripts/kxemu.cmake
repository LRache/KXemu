# LLVM
find_package(LLVM REQUIRED CONFIG)

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    MC
    Support
    ${BASE_ISA_UPPER}Disassembler
)

include_directories(${LLVM_INCLUDE_DIRS})

# Readline
link_libraries(readline)

# gdbstub
include_directories(${CMAKE_SOURCE_DIR}/utils/mini-gdbstub/include)

link_libraries(
    ${LLVM_LIBS}
    ${CMAKE_SOURCE_DIR}/utils/mini-gdbstub/build/libgdbstub.a
)

add_executable(${KXEMU_TARGET}
    ${EMU_SRCS}
    ${KDB_SRCS}
)

target_compile_options(${KXEMU_TARGET} PRIVATE -flto)
target_link_options(${KXEMU_TARGET} PRIVATE -flto)
