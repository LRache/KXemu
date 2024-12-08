CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -pedantic -O3 -lreadline -Wno-unused-command-line-argument
ISA ?= RISCV32
ISA_LOWER = $(shell echo $(ISA) | tr A-Z a-z)

SRC_DIR = $(abspath ./src)
BUILD_DIR = $(abspath ./build)
OBJ_DIR = $(abspath ./build/obj)

SRCS = $(abspath $(shell find $(SRC_DIR) -name "*.cpp"))
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

INCPATH += $(abspath ./include)
INCPATH += $(abspath ./tests/isa/$(ISA_LOWER))
INCFLAGS = $(addprefix -I, $(INCPATH))

CXXFLAGS += $(INCFLAGS)
CXXFLAGS += -DISA=$(ISA)

TARGET = $(BUILD_DIR)/$(ISA_LOWER)-kxemu

$(TARGET): $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

kxemu: $(TARGET)

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: kxemu run clean
