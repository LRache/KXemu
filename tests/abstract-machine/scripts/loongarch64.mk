AM_ISA_NAME = loongarch
KXEMU_ISA = loongarch64

CROSS_COMPILE ?= loongarch64-unknown-linux-gnu

COMPILE_FLAGS += -march=loongarch64

LDFLAGS += -T $(AM_DIR)/scripts/linker.ld \
           --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0 \
           -melf64loongarch

QEMU = qemu-system-riscv64
QEMU_FLAGS += -machine virt -nographic -bios none -semihosting