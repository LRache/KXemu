AM_DIR   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WORK_DIR := $(shell pwd)

ifneq ($(findstring qemu,$(MAKECMDGOALS)),)
    PLATFORM ?= qemu
else
	PLATFORM ?= kxemu
endif

ARCH = $(PLATFORM)-$(ISA)
ARCH_NAME = $(ARCH)-$(NAME)

ELF     = $(BUILD_DIR)/$(ARCH_NAME).elf
ARCHIVE = $(BUILD_DIR)/$(ARCH_NAME).a
ASM     = $(BUILD_DIR)/$(ARCH_NAME).txt

BUILD_DIR := build
OBJ_DIR   := build/$(ARCH)

OBJS += $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(SRCS))))
DEPS = $(OBJS:.o=.d)

-include $(DEPS)

# Libaries

LIBS += am klib
LIBS_ARCHIVE = $(foreach name, $(LIBS), $(AM_DIR)/$(name)/build/$(ARCH)-$(name).a)

# Kxemu configuration
KXEMU = $(AM_DIR)/../../build/$(KXEMU_ISA)-kxemu
KXEMU_FLAGS += --elf $(abspath $(ELF)) --source $(AM_DIR)/scripts/kdb/init.kdb
KXEMU_FLAGS += $(addprefix --source , $(KDB_INIT_SRC))

# Load ISA specific configuration
-include $(AM_DIR)/scripts/$(ISA).mk

CC  = $(CROSS_COMPILE)-gcc
CXX = $(CROSS_COMPILE)-g++
AS  = $(CROSS_COMPILE)-gcc
LD  = $(CROSS_COMPILE)-ld
AR  = $(CROSS_COMPILE)-ar
OBJDUMP = $(CROSS_COMPILE)-objdump

INC_PATH += $(foreach name, $(LIBS), $(AM_DIR)/$(name)/include)

CFLAGS += -nostdlib -fno-stack-protector -O3 -fno-builtin -fno-asynchronous-unwind-tables
CFLAGS += $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(CFLAGS) -fno-exceptions

ifeq ($(PLATFORM),qemu)
	COMPILE_FLAGS += -D__QEMU__
else ifeq ($(PLATFORM),kxemu)
	COMPILE_FLAGS += -D__KXEMU__
else
	$(error "Unknown platform $(PLATFORM)")
endif

ifneq ($(CORES),)
	KXEMU_FLAGS += --cores $(CORES)
	QEMU_FLAGS  += -smp $(CORES)
endif

CFLAGS += -D__PLATFORM__=$(PLATFORM)
ASFLAGS += -D__PLATFORM__=$(PLATFORM)

all: elf archive

$(OBJ_DIR)/%.o: %.c
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	$(info + AS $<)
	@ mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(COMPILE_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
	$(info + AS $<)
	@ mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(COMPILE_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cc
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CXX) $(ASFLAGS) $(CXXFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(ELF): $(OBJS) $(LIBS)
	$(info + LD $@)
	@ $(LD) $(OBJS) $(LIBS_ARCHIVE) -o $@ $(LDFLAGS)
	@ $(OBJDUMP) -d $@ > $(ASM)

$(LIBS): %:
	$(info + MAKELIB $@)
	@ make -s -C $(AM_DIR)/$* NAME=$* ISA=$(ISA) PLATFORM=$(PLATFORM) archive

$(ARCHIVE): $(OBJS)
	$(info + AR $@)
	@ $(AR) rcs $@ $(OBJS)

elf: $(ELF)
archive: $(ARCHIVE)

run: $(ELF)
	$(info + RUNNING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS) --source $(AM_DIR)/scripts/kdb/run.kdb

debug: $(ELF)
	$(info + DEBUGGING KXEMU)
	@ $(KXEMU) $(KXEMU_FLAGS) --source $(AM_DIR)/scripts/kdb/debug.kdb

gdb: $(ELF)
	$(info + DEBUGGING GDB)
	@ gdb $(KXEMU) -ex "set args $(KXEMU_FLAGS) --source $(AM_DIR)/scripts/kdb/run.kdb" 

qemu: $(ELF)
	$(info + RUNNING QEMU)
	@ $(QEMU) $(QEMU_FLAGS) -kernel $(ELF)

qemu-gdb: $(ELF)
	$(info + DEBUGGING QEMU)
	@ $(QEMU) $(QEMU_FLAGS) -kernel $(ELF) -s -S

perf: $(ELF)
	$(info + PROFILING KXEMU)
	perf record -g $(KXEMU) $(KXEMU_FLAGS) --source $(AM_DIR)/scripts/kdb/run.kdb

clean:
	rm -rf $(BUILD_DIR)
