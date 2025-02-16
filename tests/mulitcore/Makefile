ISA = riscv32
SRCS += $(shell find ./$(NAME)/src -name '*.S' -or -name '*.c')

KDB_FILE = $(NAME).kdb

ifneq ($(wildcard $(KDB_FILE)),)
	KDB_INIT_SRC += $(abspath $(KDB_FILE))
endif
KDB_INIT_SRC += $(abspath ./uart.kdb)

-include $(NAME)/Makefile

KXEMU_FLAGS += --core $(CORECOUNT)
QEMU_FLAGS += -smp 4

include ../abstract-machine/Makefile
