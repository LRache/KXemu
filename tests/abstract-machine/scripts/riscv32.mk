CROSS_COMPILE ?= riscv64-linux-gnu

COMPILE_FLAGS += -march=rv32im_zicsr -mabi=ilp32

LDFLAGS += -T $(AM_DIR)/scripts/linker.ld \
           --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0 \
           -melf32lriscv
