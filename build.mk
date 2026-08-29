KXEMU_HOME := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

include $(KXEMU_HOME)/scripts/build-config.mk

BUILD_DIR ?= $(KXEMU_HOME)/build
CMAKE ?= cmake
CMAKE_GENERATOR ?= Ninja
CMAKE_ARGS ?=

CMAKE_CONFIG_ARGS = $(foreach option,$(KXEMU_CONFIG_VARS),"-D$(option)=$(call kxemu-unquote,$($(option)))")

all: kxemu

configure:
	@ $(CMAKE) -G "$(CMAKE_GENERATOR)" \
		-S $(KXEMU_HOME) \
		-B $(BUILD_DIR) \
		$(CMAKE_CONFIG_ARGS) \
		$(CMAKE_ARGS)

kxemu: configure
	@ $(CMAKE) --build $(BUILD_DIR)

run: kxemu
	@ $(CMAKE) --build $(BUILD_DIR) --target run

clean:
	@ $(CMAKE) --build $(BUILD_DIR) --target clean

clean-all:
	@ $(CMAKE) --build $(BUILD_DIR) --target clean-all
	@ rm -rf $(BUILD_DIR)

.PHONY: all configure kxemu run clean clean-all
