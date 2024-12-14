SRC_DIR   := $(abspath ./src)
BUILD_DIR := $(abspath ./build)
OBJ_DIR   := $(BUILD_DIR)/$(ISA)/obj

ARCHIVE := $(BUILD_DIR)/$(ISA)-am.a

SRCS += $(shell find $(SRC_DIR) -path $(SRC_DIR)/isa -prune -o \( -name "*.cpp" -o -name "*.c" \) -print) \
        $(shell find $(SRC_DIR)/isa/$(ISA) -name "*.cpp" -o -name "*.c" -o -name "*.s")

-include $(DEPS)

include ../makefile # Path: tests/cpu-tests/abstract-machine/makefile
