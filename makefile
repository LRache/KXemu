CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -pedantic -O3 -Wno-unused-command-line-argument -Wno-unused-parameter -Wno-unused-private-field -flto
ISA ?= RISCV32
ISA_LOWER = $(shell echo $(ISA) | tr A-Z a-z)

SRC_DIR = ./src
BUILD_DIR = ./build
OBJ_DIR = ./build/obj

SRCS += $(shell find $(SRC_DIR) -path $(SRC_DIR)/isa -prune -o \( -name "*.cpp" -o -name "*.c" \) -print)
SRCS += $(shell find $(SRC_DIR)/isa/$(ISA_LOWER) -name "*.cpp" -or -name "*.c" )
OBJS += $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

INCPATH += $(abspath ./include)
INCPATH += $(abspath ./tests/isa/$(ISA_LOWER))
INCFLAGS = $(addprefix -I,$(INCPATH))

LIBS += readline
LIBFLAGS = $(addprefix -l,$(LIBS))

CXXFLAGS += $(INCFLAGS)
CXXFLAGS += $(LIBFLAGS)
CXXFLAGS += -DISA=$(ISA)

# for llvm
CXXFLAGS += $(shell llvm-config --cxxflags) -fexceptions # for expection handling
CXXFLAGS += $(shell llvm-config --libs)

ifeq ($(shell which ccache > /dev/null 2>&1; echo $$?), 0)
	CXX := ccache $(CXX)
endif

TARGET = $(BUILD_DIR)/$(ISA_LOWER)-kxemu

$(TARGET): $(OBJS)
	$(info + CXX $@)
	@ mkdir -p $(BUILD_DIR)
	@ $(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS) $(CXXFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(info + CXX $<)
	@ mkdir -p $(dir $@)
	@ $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

kxemu: $(TARGET)

run: $(TARGET)
	$(info + Running $(TARGET))
	@ $(TARGET) $(FLAGS)

clean:
	rm -rf $(BUILD_DIR)

count:
	$(info Counting lines in src and include directories...)
	@find $(SRC_DIR) ./include -name '*.c' -or -name "*.cpp" -or -name "*.h" | xargs cat | sed '/^\s*$$/d' | wc -l

tidy:
	clang-tidy $(SRCS)

.PHONY: kxemu run clean
