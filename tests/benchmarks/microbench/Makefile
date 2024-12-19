NAME = microbench
SRCS = $(shell find src/ -name "*.c" -o -name "*.cc")
SRC_DIR = src
ELF = build/$*-$(ISA).elf
INC_PATH += $(abspath ./include)

KDB_INIT_SRC = $(abspath ./init.kdb)

include ../../abstract-machine/makefile
