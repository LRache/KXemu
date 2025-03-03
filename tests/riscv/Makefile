ISA = riscv32
SRCS += $(shell find ./$(NAME)/src -name '*.S' -or -name '*.c')

KDB_FILE = ./$(NAME)/$(NAME).kdb

ifneq ($(wildcard $(KDB_FILE)),)
	KDB_INIT_SRC += $(abspath $(KDB_FILE))
else
	KDB_INIT_SRC += $(abspath ./uart.kdb)
endif

-include ./$(NAME)/Makefile
include ../abstract-machine/Makefile
