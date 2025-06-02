CROSS_COMPILE = riscv64-linux-gnu-
BUSYBOX_VERSION = 1.36.0
BUSYBOX_DIR = ./busybox-$(BUSYBOX_VERSION)/_install
BUSYBOX = $(BUSYBOX_DIR)/bin/busybox
BUSYBOX_TAR = ./busybox-$(BUSYBOX_VERSION).tar.bz2
BUSYBOX_SRC = ./busybox-$(BUSYBOX_VERSION)/Makefile

INIT_EXEC = init
INITRAMFS = initramfs.cpio.gz

all: initramfs

$(BUSYBOX_SRC):
	$(info + DOWNLOADING BUSYBOX)
	@ wget https://busybox.net/downloads/busybox-$(BUSYBOX_VERSION).tar.bz2 -O $(BUSYBOX_TAR)
	$(info + EXTRACTING BUSYBOX)
	@ tar -xjf $(BUSYBOX_TAR)

$(BUSYBOX):  $(BUSYBOX_SRC)
	$(info + BUILDING BUSYBOX)
	# @ make -C busybox-1.36.0 ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) defconfig
	# @ cp ./busybox_defconfig busybox-$(BUSYBOX_VERSION)/.config
	@ make -C busybox-1.36.0 ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) CFLAGS="-march=rv64g" install -j$(shell nproc)

$(INITRAMFS): $(BUSYBOX) $(INIT_EXEC)
	$(info + CREATING INITRAMFS)
	@ rm -rf initramfs
	@ cp -r $(BUSYBOX_DIR) initramfs/
	@ cp $(INIT_EXEC) initramfs/
	@ mkdir -p initramfs/dev
	@ mkdir -p initramfs/proc
	@ mkdir -p initramfs/sys
	@ cd initramfs && find . | cpio -H newc -o | gzip -9 > ../$(INITRAMFS)

initramfs: $(INITRAMFS)
