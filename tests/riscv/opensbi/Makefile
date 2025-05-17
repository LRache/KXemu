OPENSBI = ./opensbi/build/platform/generic/firmware/fw_payload.bin
CROSS_COMPILE = riscv64-linux-gnu-

KXEMU = ../../../build/kxemu-system-riscv64
KXEMU_FLAGS += --source $(abspath ./opensbi.kdb)
KXEMU_FLAGS += --core 1
KXEMU_FLAGS += --def openSBIBIN=$(OPENSBI)

all: run

$(OPENSBI):
	$(info + BUILDING OPENSBI)
	@ $(MAKE) -C opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE)

run: $(OPENSBI)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

qemu: $(OPENSBI)
	$(info + RUNNING QEMU)
	qemu-system-riscv64 -M virt -m 256M -nographic -bios $(OPENSBI)

qemu-gdb: $(OPENSBI)
	$(info + RUNNING QEMU GDB)
	qemu-system-riscv64 -M virt -m 256M -nographic -bios $(OPENSBI) -s -S

clean:
	$(info + CLEANING OPENSBI)
	@ $(MAKE) -C opensbi clean

.PHONY: run
