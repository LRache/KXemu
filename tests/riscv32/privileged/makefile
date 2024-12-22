ISA=riscv32
SRCS += $(shell find $(NAME) -name '*.S')

KDB_FILE = $(NAME).kdb

ifneq ($(wildcard $(KDB_FILE)),)
	KDB_INIT_SRC += $(abspath $(KDB_FILE))
endif
KDB_INIT_SRC += $(abspath ./uart.kdb)

include ../../abstract-machine/makefile
