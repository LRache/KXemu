SRC_DIR   := $(abspath ./src)
BUILD_DIR := $(abspath ./build)
OBJ_DIR   := $(BUILD_DIR)/$(ISA)/obj

ARCHIVE := $(BUILD_DIR)/$(ISA)-klib.a

SRCS += $(shell find $(SRC_DIR) -name "*.cpp" -o -name "*.c")

include ../makefile # Path: tests/cpu-tests/abstract-machine/makefile
