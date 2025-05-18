XLEN ?= 64

OPENSBI = ./build/fw_payload-$(XLEN).bin
CROSS_COMPILE = riscv64-linux-gnu-

KXEMU = ../../../build/kxemu-system-riscv$(XLEN)
KXEMU_FLAGS += --source $(abspath ./opensbi.kdb)
KXEMU_FLAGS += --core 1
KXEMU_FLAGS += --def openSBIBIN=$(OPENSBI)

QEMU = qemu-system-riscv$(XLEN)

all: run

$(OPENSBI):
	$(info + BUILDING OPENSBI)
	@ $(MAKE) -C opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM_RISCV_XLEN=$(XLEN)
	@ mkdir -p ./build
	@ cp ./opensbi/build/platform/generic/firmware/fw_payload.bin $(OPENSBI)

run: $(OPENSBI)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

qemu: $(OPENSBI)
	$(info + RUNNING QEMU)
	$(QEMU) -M virt -m 256M -nographic -bios $(OPENSBI)

qemu-gdb: $(OPENSBI)
	$(info + RUNNING QEMU GDB)
	$(QEMU) -M virt -m 256M -nographic -bios $(OPENSBI) -s -S

clean:
	$(info + CLEANING OPENSBI)
	@ $(MAKE) -C opensbi clean
	@ rm -rf ./build

.PHONY: run
