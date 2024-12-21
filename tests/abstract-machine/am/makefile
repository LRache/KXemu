SRC_DIR = ./src

NAME = am

SRCS += $(shell find $(SRC_DIR) -path $(SRC_DIR)/isa -prune -o \( -name "*.cpp" -o -name "*.c" \) -print) \
        $(shell find $(SRC_DIR)/isa/$(ISA) -name "*.cpp" -o -name "*.c" -o -name "*.S")

include ../makefile # Path: tests/cpu-tests/abstract-machine/makefile
