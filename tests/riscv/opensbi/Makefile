OPENSBI = ./opensbi/build/platform/generic/firmware/fw_jump.elf
CROSS_COMPILE = riscv64-linux-gnu-

KXEMU = ../../../build/kxemu-system-riscv64
KXEMU_FLAGS += --source $(abspath ./opensbi.kdb)
KXEMU_FLAGS += --core 1
KXEMU_FLAGS += --def elf=$(OPENSBI)

all: run

$(OPENSBI): $(KERNEL_TARGET)
	$(info + BUILDING OPENSBI)
	@ $(MAKE) -C opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE)

run: $(KERNEL_TARGET) $(OPENSBI)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

clean:
	$(info + CLEANING OPENSBI)
	@ $(MAKE) -C opensbi clean

.PHONY: run
