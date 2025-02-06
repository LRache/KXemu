SRCS += ./$(NAME).c
SRC_DIR = .
ELF = build/$(NAME)-$(ISA).elf

KDB_FILE = $(NAME).kdb

ifneq ($(wildcard $(KDB_FILE)),)
	KDB_INIT_SRC += $(abspath $(KDB_FILE))
endif

include ../abstract-machine/Makefile
