KERNEL_IMAGE = ../../../os/linux-5.15.132/arch/riscv/boot/Image
OPENSBI_BIN = ./opensbi/build/platform/generic/firmware/fw_jump.bin
SYMBOL_ELF = ../../../os/linux-5.15.132/vmlinux

QEMU = qemu-system-riscv64
QEMU_FLAGS += -M virt -m 256M -nographic -kernel $(KERNEL_IMAGE)
QEMU_FLAGS += -bios $(OPENSBI_BIN)

GDB = gdb-multiarch

CROSS_COMPILE = riscv64-linux-gnu-
XLEN = 64

DTS = kxemu-virt-riscv64.dts
DTB = kxemu-virt-riscv64.dtb

KXEMU = ../../../../build/kxemu-system-riscv64
KXEMU_FLAGS += --source $(abspath ./kxemu-riscv64.kdb)
KXEMU_FLAGS += --core 1
KXEMU_FLAGS += --def kernel=$(KERNEL_IMAGE) 
KXEMU_FLAGS += --def openSBI=$(OPENSBI_BIN) 
KXEMU_FLAGS += --def symbolELF=$(SYMBOL_ELF)
KXEMU_FLAGS += --def dtb=$(DTB)

all: run

$(KERNEL_IMAGE):
	$(info + COPYING CONFIG)
	@ cp ./defconfig ../../../os/linux-5.15.132
	@ make -C ../../../os/linux-5.15.132 ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) olddefconfig
	$(info + BUILDING KERNEL)
	@ make -C ../../../os/linux-5.15.132 ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) -j$(shell nproc) all

kernel: $(KERNEL_IMAGE)

qemu: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB)
	$(info + RUNNING QEMU)
	@ $(QEMU) $(QEMU_FLAGS)

qemu-dbg: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB)
	@ $(QEMU) $(QEMU_FLAGS) -s -S

$(OPENSBI_BIN):
	$(MAKE) -C opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM_RISCV_XLEN=$(XLEN)

$(DTB): 
	$(info + GENERATING DTB)
	@ dtc -I dts -O dtb -o $(DTB) $(DTS)

run: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

kxemu: run

kxemu-dbg: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB)
	$(info + RUNNING GDB)
	@ $(GDB) -x ./kxemu.gdb

qemu-dtb:
	@ $(QEMU) -machine virt,dumpdtb=qemu-virt-riscv64.dtb -kernel $(KERNEL_IMAGE) -bios $(OPENSBI_BIN)
	@ dtc -I dtb -O dts -o qemu-virt-riscv64.dts qemu-virt-riscv64.dtb
