AM_ISA_NAME = riscv
KXEMU_ISA = riscv64

CROSS_COMPILE ?= riscv64-linux-gnu

COMPILE_FLAGS += -march=rv64ima_zicsr -mabi=lp64

LDFLAGS += -T $(AM_DIR)/scripts/linker.ld \
           --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0 \
           -melf64lriscv

QEMU = qemu-system-riscv64
QEMU_FLAGS += -machine virt,aclint=on -nographic -bios none -semihosting
