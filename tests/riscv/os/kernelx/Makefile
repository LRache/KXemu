# Build KernelX (riscv64, dev branch) and boot it on kxemu-system-riscv64.
#
# The KernelX source is the tests/os/KernelX submodule. The kernel image is
# loaded by OpenSBI's fw_jump firmware at 0x80200000, exactly like the Linux
# test under ../linux5.15. The root filesystem is an ext4 image served by
# KXemu's virtio-mmio block device.
#
# KXemu halts the hart on every ebreak (CONFIG_HALT_WHEN_BREAK), and OpenSBI's
# default generic build probes for semihosting by executing an ebreak. This
# Makefile therefore builds a private OpenSBI with CONFIG_SERIAL_SEMIHOSTING=n.

KXEMU_HOME := $(abspath ../../../..)
KERNELX_HOME := $(abspath ../../../os/KernelX)

KXEMU ?= $(KXEMU_HOME)/build/kxemu-system-riscv64

OPENSBI_SRC ?= ../../opensbi/opensbi
OPENSBI_BUILD := build/opensbi
OPENSBI_BIN := $(OPENSBI_BUILD)/platform/generic/firmware/fw_jump.bin
OPENSBI_FLAGS := \
	PLATFORM=generic \
	PLATFORM_RISCV_XLEN=64 \
	FW_DYNAMIC=n \
	FW_PAYLOAD=n \
	FW_JUMP=y

KERNELX_BUILD := $(KERNELX_HOME)/build/riscv64
KERNEL_IMAGE := $(KERNELX_BUILD)/Image
SYMBOL_ELF := $(KERNELX_BUILD)/vmkernelx

BUILD_DIR := build
SMP ?= 4
DTS := $(BUILD_DIR)/kernelx-riscv64.dts
DTB := $(BUILD_DIR)/kernelx-riscv64.dtb
ROOTFS := $(BUILD_DIR)/rootfs.ext4
ROOTFS_DIR := $(BUILD_DIR)/rootfs

CROSS_COMPILE ?= $(shell command -v riscv64-unknown-linux-gnu-gcc >/dev/null 2>&1 && \
	echo riscv64-unknown-linux-gnu- || \
	echo /opt/cross-tools/riscv64-unknown-linux-gnu/bin/riscv64-unknown-linux-gnu-)
MUSL_CC ?= $(shell command -v riscv64-linux-musl-gcc 2>/dev/null || \
	echo /opt/cross-tools/riscv64-linux-musl/bin/riscv64-linux-musl-gcc)
# A host objcopy cannot convert a RISC-V ELF, so prefer LLVM's.
OBJCOPY ?= $(shell command -v llvm-objcopy 2>/dev/null || \
	command -v llvm-objcopy-21 2>/dev/null || echo objcopy)

KERNELX_FLAGS := \
	ARCH=riscv \
	ARCH_BITS=64 \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	KERNELX_RELEASE=5.0 \
	COMPILE_MODE=release \
	CONFIG_LOG_LEVEL=info \
	CONFIG_WARN_UNIMPLEMENTED_SYSCALL=y \
	CONFIG_VIRTIO_BLOCK_PAGE_CACHE=n \
	OBJCOPY=$(OBJCOPY)

KXEMU_FLAGS := \
	--source $(abspath ./kernelx-riscv64.kdb) \
	--core $(SMP) \
	--def dtb=$(abspath $(DTB)) \
	--def openSBI=$(abspath $(OPENSBI_BIN)) \
	--def kernel=$(KERNEL_IMAGE) \
	--def symbolELF=$(SYMBOL_ELF) \
	--def rootfs=$(abspath $(ROOTFS))

all: kernel

kernel:
	$(info + BUILDING KERNELX)
	@ $(MAKE) -C $(KERNELX_HOME) -f build.mk kernel $(KERNELX_FLAGS)

$(OPENSBI_BIN):
	$(info + BUILDING OPENSBI)
	@ mkdir -p $(OPENSBI_BUILD)/platform/generic/kconfig
	@ cd $(abspath $(OPENSBI_SRC))/scripts/Kconfiglib && \
		KCONFIG_CONFIG=$(abspath $(OPENSBI_BUILD))/platform/generic/kconfig/.config \
		OPENSBI_SRC_DIR=$(abspath $(OPENSBI_SRC)) \
		OPENSBI_PLATFORM=generic \
		OPENSBI_PLATFORM_SRC_DIR=$(abspath $(OPENSBI_SRC))/platform/generic \
		python3 defconfig.py --kconfig $(abspath $(OPENSBI_SRC))/Kconfig \
		$(abspath $(OPENSBI_SRC))/platform/generic/configs/defconfig
	@ sed -i 's/^CONFIG_SERIAL_SEMIHOSTING=y$$/CONFIG_SERIAL_SEMIHOSTING=n/' \
		$(OPENSBI_BUILD)/platform/generic/kconfig/.config
	@ $(MAKE) -C $(OPENSBI_SRC) O=$(abspath $(OPENSBI_BUILD)) $(OPENSBI_FLAGS) \
		CROSS_COMPILE=$(CROSS_COMPILE)

$(DTS): gen-dts.py FORCE
	$(info + GENERATING DTS)
	@ mkdir -p $(BUILD_DIR)
	@ python3 gen-dts.py --smp $(SMP) --output $@

$(DTB): $(DTS)
	$(info + GENERATING DTB)
	@ dtc -I dts -O dtb -o $@ $<

$(ROOTFS): $(KERNELX_HOME)/usertests/init/init.c initramfs/hello.c
	$(info + BUILDING ROOTFS)
	@ mkdir -p $(ROOTFS_DIR)
	@ $(MUSL_CC) -static -O2 -Wall -Wextra -o $(ROOTFS_DIR)/init $(KERNELX_HOME)/usertests/init/init.c
	@ $(MUSL_CC) -static -O2 -Wall -Wextra -o $(ROOTFS_DIR)/hello initramfs/hello.c
	@ rm -f $(ROOTFS)
	@ truncate -s 64M $(ROOTFS)
	@ mke2fs -q -t ext4 -b 4096 -F -d $(ROOTFS_DIR) $(ROOTFS)

rootfs: $(ROOTFS)

run: kernel $(OPENSBI_BIN) $(DTB) $(ROOTFS)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS)

clean:
	$(info + CLEANING)
	@ rm -rf $(BUILD_DIR)
	@ $(MAKE) -C $(KERNELX_HOME) -f build.mk clean $(KERNELX_FLAGS)

.PHONY: all kernel rootfs run clean FORCE

FORCE:
