KXEMU = ../../build/riscv64-kxemu
KXEMU_FLAGS += --source $(abspath ./xv6.kdb)
KXEMU_FLAGS += --core 1
KXEMU_FLAGS += --def elf=$(KERNEL_TARGET) --def fsimg=${FS_IMG}

KERNEL_TARGET = ./xv6-riscv/kernel/kernel
FS_IMG = ./xv6-riscv/fs.img

$(KERNEL_TARGET):
	make -C ./xv6-riscv kernel/kernel

$(FS_IMG):
	make -C ./xv6-riscv fs.img

run: $(KERNEL_TARGET) $(FS_IMG)
	$(KXEMU) $(KXEMU_FLAGS)
