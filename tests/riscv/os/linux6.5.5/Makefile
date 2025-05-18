KERNEL_IMAGE = ../../../os/linux-6.5.5/arch/riscv/boot/Image
OPENSBI_BIN = ./opensbi/build/platform/generic/firmware/fw_jump.bin

QEMU = qemu-system-riscv64
QEMU_FLAGS += -M virt -m 256M -nographic -kernel $(KERNEL_IMAGE)

CROSS_COMPILE = riscv64-linux-gnu-
XLEN = 64

KXEMU = ../../../../build/kxemu-system-riscv64
KXEMU_FLAGS += --source $(abspath ./kxemu-riscv64.kdb)
KXEMU_FLAGS += --core 1
KXEMU_FLAGS += --def kernel=$(KERNEL_IMAGE)
KXEMU_FLAGS += --def openSBI=$(OPENSBI_BIN)

all: run

qemu:
	$(info + RUNNING QEMU)
	@ $(QEMU) $(QEMU_FLAGS)

qemu-dbg: 
	@ $(QEMU) $(QEMU_FLAGS) -s -S

$(OPENSBI_BIN):
	$(MAKE) -C opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM_RISCV_XLEN=$(XLEN)

run: $(OPENSBI_BIN)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)
