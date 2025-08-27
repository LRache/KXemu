KXEMU = ../../../build/kxemu-system-riscv64
KXEMU_FLAGS += --source $(abspath ./xv6.kdb)
KXEMU_FLAGS += --core 3
KXEMU_FLAGS += --def elf=$(KERNEL_TARGET) --def fsimg=${FS_IMG}

KERNEL_TARGET = ./xv6-riscv/kernel/kernel
FS_IMG = ./xv6-riscv/fs.img

$(KERNEL_TARGET):
	$(info + MAKE kernel/kernel)
	@ make -s -C ./xv6-riscv kernel/kernel

$(FS_IMG):
	$(info + MAKE fs.img)
	@ make -s -C ./xv6-riscv fs.img

run: $(KERNEL_TARGET) $(FS_IMG)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

clean:
	$(info + CLEANING)
	@ make -s -C ./xv6-riscv clean

.PHONY: $(KERNEL_TARGET) $(FS_IMG) clean
.DEFAULT_GOAL := run
