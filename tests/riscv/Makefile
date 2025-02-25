ISA = riscv32
SRCS += $(shell find ./$(NAME)/src -name '*.S' -or -name '*.c')

KDB_FILE = $(NAME).kdb

ifneq ($(wildcard $(KDB_FILE)),)
	KDB_INIT_SRC += $(abspath $(KDB_FILE))
endif
KDB_INIT_SRC += $(abspath ./uart.kdb)

-include ./$(NAME)/Makefile
include ../abstract-machine/Makefile
