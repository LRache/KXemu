.DEFAULT_GOAL := all
KXEMU_HOME := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SCRIPT_DIR := $(KXEMU_HOME)/scripts
SRC_DIR := $(KXEMU_HOME)/src
INCLUDE_DIR := $(KXEMU_HOME)/include
BUILD_DIR := $(KXEMU_HOME)/build
EXPORT_DIR := $(KXEMU_HOME)/export
include $(SCRIPT_DIR)/config.mk
include $(SCRIPT_DIR)/build-config.mk

KXEMU_CONFIG = $(foreach option,$(KXEMU_CONFIG_VARS),$(option)='$(call kxemu-unquote,$($(option)))')

all: kxemu

count:
	$(info Counting lines in src and include directories...)
	@ find -L $(SCRIPT_DIR) $(SRC_DIR) $(INCLUDE_DIR) -type f \( -name '*.c' -or -name "*.cpp" -or -name "*.h" -or -name "*.hpp" -or -name "*.py" -or -name "*.instpat" -or -name "*.cmake" \) -exec cat {} + | sed '/^[[:space:]]*$$/d' | wc -l

kxemu:
	@ $(MAKE) -f $(KXEMU_HOME)/build.mk kxemu $(KXEMU_CONFIG)

run:
	@ $(MAKE) -f $(KXEMU_HOME)/build.mk run $(KXEMU_CONFIG)

export-include:
	@ mkdir -p $(EXPORT_DIR)
	$(info + Exporting Headers $(CONFIG_ISA))
	@ python3 $(SCRIPT_DIR)/export/export.py

clean:
	$(info + CLEAN $(BUILD_DIR))
	-@ $(MAKE) -f $(KXEMU_HOME)/build.mk clean $(KXEMU_CONFIG)

clean-all:
	$(info + CLEAN-ALL)
	-@ $(MAKE) -f $(KXEMU_HOME)/build.mk clean-all $(KXEMU_CONFIG)
	-@$(MAKE) distclean


.PHONY: all count kxemu run export-include clean clean-all
