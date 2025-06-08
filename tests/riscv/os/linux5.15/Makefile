KERNEL_IMAGE = ../../../os/linux-5.15.132/arch/riscv/boot/Image
OPENSBI_BIN = ./opensbi/build/platform/generic/firmware/fw_jump.bin
SYMBOL_ELF = ../../../os/linux-5.15.132/vmlinux

INITRAMFS = ./initramfs/initramfs.cpio.gz

QEMU = qemu-system-riscv64
QEMU_FLAGS += -M virt -cpu rv64 -nographic 
QEMU_FLAGS += -kernel $(KERNEL_IMAGE)
QEMU_FLAGS += -bios $(OPENSBI_BIN)
QEMU_FLAGS += -initrd $(INITRAMFS)

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
KXEMU_FLAGS += --def initramfs=$(INITRAMFS)

all: kxemu
run: kxemu

$(KERNEL_IMAGE):
	$(info + COPYING CONFIG)
	# @ cp ./kxemu_riscv64_defconfig ../../../os/linux-5.15.132/arch/riscv/configs/
	# @ make -C ../../../os/linux-5.15.132 ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) kxemu_riscv64_defconfig
	$(info + BUILDING KERNEL)
	@ make -C ../../../os/linux-5.15.132 ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) -j$(shell nproc) all

kernel: $(KERNEL_IMAGE)

$(INITRAMFS):
	$(info + GENERATING INITRAMFS)
	@ make -C initramfs

qemu: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(INITRAMFS)
	$(info + RUNNING QEMU)
	@ $(QEMU) $(QEMU_FLAGS) 

qemu-dbg: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB)
	@ $(QEMU) $(QEMU_FLAGS) -s -S

qemu-gdb:
	$(info + RUNNING GDB)
	@ $(GDB) -x ./qemu.gdb

$(OPENSBI_BIN):
	$(MAKE) -C opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM_RISCV_XLEN=$(XLEN)

$(DTB): $(DTS)
	$(info + GENERATING DTB)
	@ dtc -I dts -O dtb -o $(DTB) $(DTS)

kxemu: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB) $(INITRAMFS)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

kxemu-gdb: $(KERNEL_IMAGE) $(OPENSBI_BIN) $(DTB)
	$(info + RUNNING GDB)
	@ $(GDB) -x ./kxemu.gdb

qemu-dts:
	@ $(QEMU) -machine virt,dumpdtb=qemu-virt-riscv64.dtb -kernel $(KERNEL_IMAGE) -bios $(OPENSBI_BIN) -initrd $(INITRAMFS)
	@ dtc -I dtb -O dts -o qemu-virt-riscv64.dts qemu-virt-riscv64.dtb

.PHONY: $(KERNEL_IMAGE) $(INITRAMFS)
