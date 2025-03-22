CXX = clang++

BUILD_DIR = ./build
TARGET = $(BUILD_DIR)/main
KXEMU_LIB = $(abspath ../../build/libkxemu-riscv64.a)

INCPATH += $(abspath ../../export/riscv64/include)
INCFLAGS = $(addprefix -I,$(INCPATH))
CXXFLAGS += $(INCFLAGS)

$(TARGET): main.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $(TARGET) main.cpp $(KXEMU_LIB) $(CXXFLAGS)

run: $(TARGET)
	$(TARGET)

.DEFAULT_GOAL := run
