AM_ISA_NAME = riscv

CROSS_COMPILE ?= riscv64-linux-gnu

COMPILE_FLAGS += -march=rv64im_zicsr -mabi=lp64

LDFLAGS += -T $(AM_DIR)/scripts/linker.ld \
           --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0 \
           -melf64lriscv

QEMU = qemu-system-riscv64
QEMU_FLAGS = -machine virt -nographic -bios none -semihosting
