# Set abstract machine code configuration

AM_DIR 	   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
AM_SRC_DIR := $(AM_DIR)/src

## abstarct machine source code except isa
AM_SRC += $(shell find $(AM_SRC_DIR) -path $(AM_SRC_DIR)/isa -prune -o \( -name "*.cpp" -o -name "*.c" \) -print)

## abstarct machine source code for isa
AM_SRC += $(shell find $(AM_SRC_DIR)/isa/$(ISA) -name "*.cpp" -o -name "*.c" -o -name "*.s")

AM_TARGET_DIR = $(AM_DIR)/build/$(ISA)
AM_OBJ_DIR = $(AM_TARGET_DIR)/obj

AM_OBJS += $(patsubst $(AM_SRC_DIR)/%.cpp, $(AM_OBJ_DIR)/%.o, $(filter $(AM_SRC_DIR)/%.cpp, $(AM_SRC))) \
		   $(patsubst $(AM_SRC_DIR)/%.c, $(AM_OBJ_DIR)/%.o, $(filter $(AM_SRC_DIR)/%.c, $(AM_SRC))) \
           $(patsubst $(AM_SRC_DIR)/%.s, $(AM_OBJ_DIR)/%.o, $(filter $(AM_SRC_DIR)/%.s, $(AM_SRC)))
AM_DEPS = $(AM_OBJS:.o=.d)

-include $(AM_DEPS)

# User code configuration
WORD_DIR = $(shell pwd)
TARGET_DIR = $(WORD_DIR)/build/$(ISA)
OBJ_DIR = $(TARGET_DIR)/obj

OBJS += $(patsubst %.cpp, $(TARGET_DIR)/%.o, $(filter %.cpp, $(SRC))) \
		$(patsubst %.c, $(TARGET_DIR)/%.o, $(filter %.c, $(SRC))) \
		$(patsubst %.s, $(TARGET_DIR)/%.o, $(filter %.s, $(SRC)))
DEPS = $(OBJS:.o=.d)

-include $(DEPS)

# Kxemu configuration
KXEMU = $(AM_DIR)/../../build/$(ISA)-kxemu
KXEMU_FLAGS += --elf $(abspath $(TARGET)) --source $(AM_DIR)/scripts/dummy.kdb

# Load ISA specific configuration

include $(AM_DIR)/scripts/$(ISA).mk

CC  = $(CROSS_COMPILE)-gcc
CXX = $(CROSS_COMPILE)-g++
AS  = $(CROSS_COMPILE)-gcc
LD  = $(CROSS_COMPILE)-ld

CFLAGS += -nostdlib -fno-stack-protector

$(AM_OBJ_DIR)/%.o: $(AM_SRC_DIR)/%.c
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(AM_OBJ_DIR)/%.o: $(AM_SRC_DIR)/%.s
	$(info + AS $<)
	@ mkdir -p $(dir $@)
	@ $(AS) $(COMPILE_FLAGS) -c $< -o $@

$(AM_OBJ_DIR)/%.o: $(AM_SRC_DIR)/%.cpp
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(TARGET_DIR)/%.o: %.c
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(TARGET_DIR)/%.o: %.s
	$(info + AS $<)
	@ mkdir -p $(dir $@)
	@ $(AS) $(COMPILE_FLAGS) -c $< -o $@

$(TARGET_DIR)/%.o: %.cpp
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(AM_OBJS) $(OBJS)
	$(info + LD $@)
	mkdir -p $(TARGET_DIR)
	$(LD) $(AM_OBJS) $(OBJS) -o $@ $(LDFLAGS)

run: $(TARGET)
	$(KXEMU) $(KXEMU_FLAGS)

clean:
	rm -rf $(TARGET_DIR) $(AM_OBJ_DIR)
