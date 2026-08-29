# Configuration values shared by the menuconfig and direct build.mk paths.

kxemu-unquote = $(patsubst "%",%,$(strip $(1)))

KXEMU_CONFIG_VARS := \
	CONFIG_ISA_riscv32 \
	CONFIG_ISA_riscv64 \
	CONFIG_ISA_loongarch32r \
	CONFIG_ISA_loongarch32 \
	CONFIG_ISA_loongarch64 \
	CONFIG_ISA \
	CONFIG_BASE_ISA \
	CONFIG_ENABLE_ICACHE \
	CONFIG_ICACHE_SET_BITS \
	CONFIG_TLB \
	CONFIG_TLB_SET_BITS \
	CONFIG_DCache \
	CONFIG_FPU_HOST \
	CONFIG_FPU_SOFT \
	CONFIG_FPU_NONE \
	CONFIG_HALT_WHEN_BREAK \
	CONFIG_HALT_WHEN_INVALID \
	CONFIG_ENGINE_INTERPRETER \
	CONFIG_ENGINE \
	CONFIG_MODE_SYSTEM \
	CONFIG_TARGET_NATIVE_ELF \
	CONFIG_TARGET_SHARE \
	CONFIG_CC_GPP \
	CONFIG_CC_CLANGPP \
	CONFIG_CC \
	CONFIG_CC_LTO \
	CONFIG_CC_ADDRASAN \
	CONFIG_CC_UBASAN \
	CONFIG_CC_PGO \
	CONFIG_CC_PROFILE_INSTR_USE \
	CONFIG_LOG \
	CONFIG_HINT \
	CONFIG_DEBUG \
	CONFIG_DEBUG_DECODER \
	CONFIG_DEBUG_DCACHE \
	CONFIG_BREAK \
	CONFIG_USE_EXCEPTION

kxemu-selected-isa := $(strip \
	$(if $(filter y,$(CONFIG_ISA_riscv64)),riscv64,\
	$(if $(filter y,$(CONFIG_ISA_loongarch32r)),loongarch32r,\
	$(if $(filter y,$(CONFIG_ISA_loongarch32)),loongarch32,\
	$(if $(filter y,$(CONFIG_ISA_loongarch64)),loongarch64,\
	$(if $(filter y,$(CONFIG_ISA_riscv32)),riscv32))))))

CONFIG_ISA ?= $(if $(kxemu-selected-isa),$(kxemu-selected-isa),riscv32)
KXEMU_ISA := $(call kxemu-unquote,$(CONFIG_ISA))
CONFIG_BASE_ISA ?= $(if $(filter riscv32 riscv64,$(KXEMU_ISA)),riscv,loongarch)

CONFIG_ISA_riscv32 ?= $(if $(filter riscv32,$(KXEMU_ISA)),y,n)
CONFIG_ISA_riscv64 ?= $(if $(filter riscv64,$(KXEMU_ISA)),y,n)
CONFIG_ISA_loongarch32r ?= $(if $(filter loongarch32r,$(KXEMU_ISA)),y,n)
CONFIG_ISA_loongarch32 ?= $(if $(filter loongarch32,$(KXEMU_ISA)),y,n)
CONFIG_ISA_loongarch64 ?= $(if $(filter loongarch64,$(KXEMU_ISA)),y,n)

CONFIG_ENABLE_ICACHE ?= n
CONFIG_ICACHE_SET_BITS ?= $(if $(filter y,$(CONFIG_ENABLE_ICACHE)),11,0)
CONFIG_TLB ?= n
CONFIG_TLB_SET_BITS ?= $(if $(filter y,$(CONFIG_TLB)),5,0)
CONFIG_DCache ?= n

CONFIG_FPU_HOST ?= $(if $(filter y,$(CONFIG_FPU_SOFT) $(CONFIG_FPU_NONE)),n,y)
CONFIG_FPU_SOFT ?= n
CONFIG_FPU_NONE ?= n
CONFIG_HALT_WHEN_BREAK ?= n
CONFIG_HALT_WHEN_INVALID ?= n
CONFIG_ENGINE_INTERPRETER ?= y
CONFIG_ENGINE ?= interpreter
CONFIG_MODE_SYSTEM ?= y
CONFIG_TARGET_SHARE ?= n
CONFIG_TARGET_NATIVE_ELF ?= $(if $(filter y,$(CONFIG_TARGET_SHARE)),n,y)

kxemu-selected-cc := $(if $(filter y,$(CONFIG_CC_GPP)),g++,$(if $(filter y,$(CONFIG_CC_CLANGPP)),clang++))
CONFIG_CC ?= $(if $(kxemu-selected-cc),$(kxemu-selected-cc),clang++)
CONFIG_CC_GPP ?= $(if $(filter g++,$(call kxemu-unquote,$(CONFIG_CC))),y,n)
CONFIG_CC_CLANGPP ?= $(if $(filter clang++,$(call kxemu-unquote,$(CONFIG_CC))),y,n)
CONFIG_CC_LTO ?= n
CONFIG_CC_ADDRASAN ?= n
CONFIG_CC_UBASAN ?= n
CONFIG_CC_PGO ?= n
CONFIG_CC_PROFILE_INSTR_USE ?=

CONFIG_LOG ?= n
CONFIG_HINT ?= n
CONFIG_DEBUG ?= n
CONFIG_DEBUG_DECODER ?= n
CONFIG_DEBUG_DCACHE ?= n
CONFIG_BREAK ?= n
CONFIG_USE_EXCEPTION ?= n
