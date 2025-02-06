CONFIG_FILE = ./configs/include/config/auto.conf
CONFIG_DIR = ./configs
CONF = ./tools/kconfig/build/conf
MCONF = ./tools/kconfig/build/mconf

ifeq ($(wildcard $(CONFIG_FILE)),)
$(warning $(CONFIG_FILE) not found, run make menuconfig first)
endif

-include $(CONFIG_FILE)

$(CONF):
	@$(MAKE) -C ./tools/kconfig NAME=conf

$(MCONF):
	@$(MAKE) -C ./tools/kconfig NAME=mconf

menuconfig: $(CONF) $(MCONF)
	@ cd $(CONFIG_DIR) && ../$(MCONF) -s Kconfig
	@ cd $(CONFIG_DIR) && ../$(CONF) -s --syncconfig Kconfig
	@ mkdir -p ./include/generated
	@ cp $(CONFIG_DIR)/include/generated/autoconf.h ./include/config
