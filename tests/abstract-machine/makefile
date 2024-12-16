AM_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

BUILD_DIR = $(abspath ./build)/$(ISA)
OBJ_DIR   = $(BUILD_DIR)/obj

OBJS += $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(filter $(SRC_DIR)/%.c, $(SRCS))) \
        $(patsubst $(SRC_DIR)/%.s, $(OBJ_DIR)/%.o, $(filter $(SRC_DIR)/%.s, $(SRCS)))
DEPS = $(OBJS:.o=.d)

-include $(DEPS)

# Abstract machine libaries

LIBS += am klib
LIBS_ARCHIVE = $(foreach name, $(LIBS), $(AM_DIR)/$(name)/build/$(ISA)-$(name).a)

# Kxemu configuration
KXEMU = $(AM_DIR)/../../build/$(ISA)-kxemu
KXEMU_FLAGS += --elf $(abspath $(ELF)) --source $(AM_DIR)/scripts/dummy.kdb

# Load ISA specific configuration
-include $(AM_DIR)/scripts/$(ISA).mk

CC  = $(CROSS_COMPILE)-gcc
CXX = $(CROSS_COMPILE)-g++
AS  = $(CROSS_COMPILE)-gcc
LD  = $(CROSS_COMPILE)-ld
AR  = $(CROSS_COMPILE)-ar	

INC_PATH += $(foreach name, $(LIBS), $(AM_DIR)/$(name)/include)

CFLAGS += -nostdlib -fno-stack-protector -O2
CFLAGS += $(addprefix -I, $(INC_PATH))

$(OBJ_DIR)/%.o: %.c
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(info + CC $<)
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: %.s
	$(info + AS $<)
	@ mkdir -p $(dir $@)
	@ $(AS) $(COMPILE_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(info + AS $<)
	@ mkdir -p $(dir $@)
	@ $(AS) $(COMPILE_FLAGS) -c $< -o $@

$(ELF): $(OBJS) $(LIBS)
	$(info + LD $@)
	$(LD) $(OBJS) $(LIBS_ARCHIVE) -o $@ $(LDFLAGS)

$(LIBS): %:
	$(MAKE) -C $(AM_DIR)/$* ISA=$(ISA) archive

$(ARCHIVE): $(OBJS)
	$(info + AR $@)
	@ $(AR) rcs $@ $(OBJS)

elf: $(ELF)
archive: $(ARCHIVE)

run: $(ELF)
	$(KXEMU) $(KXEMU_FLAGS)

clean:
	rm -rf $(BUILD_DIR)
