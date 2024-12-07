CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -pedantic -O3

INCPATH += $(abspath ./include)
SRCS = $(abspath $(shell find ./src -name "*.cpp"))

INCFLAGS = $(addprefix -I, $(INCPATH))

BUILD_DIR = $(abspath ./build)
TARGET = $(BUILD_DIR)/kxemu

$(TARGET): $(SRCS) 
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(SRCS) -o $(TARGET)

kxemu: $(TARGET)

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
