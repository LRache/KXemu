CONFIG_DIR := $(KXEMU_HOME)/configs
CONFIG_FILE := $(CONFIG_DIR)/include/config/auto.conf
CONF := $(KXEMU_HOME)/tools/kconfig/build/conf
MCONF := $(KXEMU_HOME)/tools/kconfig/build/mconf
rm-distclean := $(CONFIG_DIR)/.config $(CONFIG_DIR)/.config.old $(CONFIG_DIR)/include

ifeq ($(wildcard $(CONFIG_DIR)/.config),)
$(warning Warning: $(CONFIG_DIR)/.config does not exist!)
$(warning To build the project, first run 'make menuconfig'.)
endif

-include $(CONFIG_FILE)

$(CONF):
	@ make -C $(KXEMU_HOME)/tools/kconfig NAME=conf

$(MCONF):
	@ make -C $(KXEMU_HOME)/tools/kconfig NAME=mconf

menuconfig: $(CONF) $(MCONF)
	@ cd $(CONFIG_DIR) && $(MCONF) -s Kconfig
	@ $(MAKE) syncconfig

define kconfig-syncconfig
	cd $(CONFIG_DIR) && $(CONF) -s --syncconfig Kconfig
	cp $(CONFIG_DIR)/include/generated/autoconf.h $(KXEMU_HOME)/include/config
endef

syncconfig: $(CONF)
	@ $(kconfig-syncconfig)

# Usage: make defconfig ISA=riscv64 [EXTRA_CONFIG='CONFIG_A=y CONFIG_B=y']
defconfig: $(CONF)
	@ echo "CONFIG_ISA_$(ISA)=y" > $(CONFIG_DIR)/.config
	@ for opt in $(EXTRA_CONFIG); do echo $$opt >> $(CONFIG_DIR)/.config; done
	@ $(kconfig-syncconfig)
	@ grep -q '^CONFIG_ISA="$(ISA)"$$' $(CONFIG_DIR)/.config || \
		{ echo "Error: ISA '$(ISA)' is not supported"; exit 1; }

distclean:
	-@ rm -rf $(rm-distclean)

.PHONY: distclean syncconfig defconfig
