XLEN ?= 64
SMP ?= 3

OPENSBI = ./build/fw_payload-$(XLEN).bin
CROSS_COMPILE = riscv64-linux-gnu-

KXEMU = ../../../build/kxemu-system-riscv$(XLEN)
KXEMU_FLAGS += --source $(abspath ./opensbi.kdb)
KXEMU_FLAGS += --core $(SMP)
KXEMU_FLAGS += --def openSBIBIN=$(OPENSBI)

QEMU = qemu-system-riscv$(XLEN)
QEMU_FLAGS += -M virt -m 256M -nographic
QEMU_FLAGS += -bios $(OPENSBI)
QEMU_FLAGS += -smp $(SMP)

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
	$(QEMU) $(QEMU_FLAGS)

qemu-gdb: $(OPENSBI)
	$(info + RUNNING QEMU GDB)
	$(QEMU) $(QEMU_FLAGS) -s -S

clean:
	$(info + CLEANING OPENSBI)
	@ $(MAKE) -C opensbi clean
	@ rm -rf ./build

.PHONY: run
