include ./scripts/config.mk

CXX = clang++
ifeq ($(shell which ccache > /dev/null 2>&1; echo $$?), 0)
	CXX := ccache $(CXX)
endif
LD  = clang++

remove_quote = $(patsubst "%",%,$(1))
ISA := $(call remove_quote,$(CONFIG_ISA))
BASE_ISA := $(call remove_quote,$(CONFIG_BASE_ISA))

include ./scripts/filelist.mk

OBJ_DIR = $(BUILD_DIR)/$(ISA)
OBJS += $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

INCPATH += $(abspath ./include)
INCFLAGS = $(addprefix -I,$(INCPATH))

LIBFLAGS = $(addprefix -l,$(LIBS))

CXXFLAGS = -Wall -Wextra -Werror -pedantic -Ofast -O3 -Wno-unused-command-line-argument -Wno-unused-parameter -Wno-unused-private-field
CXXFLAGS += $(INCFLAGS)

LDFLAGS += $(LIBFLAGS)

# for llvm
CXXFLAGS += $(shell llvm-config --cxxflags) -fexceptions # for expection handling
LDFLAGS  += $(shell llvm-config --libs)

-include ./scripts/isa/$(BASE_ISA).mk

ifeq ($(MAKECMDGOALS), export)
	include ./scripts/export.mk
else
	include ./scripts/kxemu.mk
endif

-include $(DEPS)
include ./scripts/build.mk

clean:
	$(info + CLEAN ./build)
	@ rm -rf ./build
	$(info + CLEAN ./export)
	@ rm -rf ./export

count:
	$(info Counting lines in src and include directories...)
	@ find $(SRC_DIR) ./include -name '*.c' -or -name "*.cpp" -or -name "*.h" | xargs cat | sed '/^\s*$$/d' | wc -l

tidy:
	clang-tidy $(SRCS)

.PHONY: kxemu run clean
.DEFAULT_GOAL := kxemu
