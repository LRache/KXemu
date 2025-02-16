AM_ISA_NAME = loongarch
KXEMU_ISA = loongarch32

CROSS_COMPILE ?= loongarch32r-linux-gnusf

LDFLAGS += -T $(AM_DIR)/scripts/linker.ld \
           --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0 \

QEMU = qemu-system-riscv64
QEMU_FLAGS += -machine virt -nographic -bios none -semihosting