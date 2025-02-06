SRC_DIR = ./src

NAME := am

SRCS += $(shell find $(SRC_DIR) -path $(SRC_DIR)/isa -prune -o \( -name "*.cpp" -o -name "*.c" \) -print) \
        $(shell find $(SRC_DIR)/isa/$(AM_ISA_NAME) -name "*.cpp" -o -name "*.c" -o -name "*.S")

include ../Makefile # Path: tests/cpu-tests/abstract-machine/Makefile

.DEFAULT_GOAL := archive
