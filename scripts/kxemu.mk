BUILD_DIR = build
SRCS += $(EMU_SRCS) $(KDB_SRCS)

LIBS += readline
INCPATH += utils/mini-gdbstub/include

ifneq ($(CONFIG_DEBUG), y)
	CXXFLAGS += -flto
	LDFLAGS  += -flto
endif

TARGET = $(BUILD_DIR)/$(ISA)-kxemu

MINIGDBSTUB = utils/mini-gdbstub/build/libgdbstub.a

UTILS_LIB += $(MINIGDBSTUB)

$(MINIGDBSTUB):
	$(info + BUILD $@)
	@ make -C utils/mini-gdbstub

$(TARGET): $(OBJS) $(UTILS_LIB)
	$(info + LD $@)
	@ mkdir -p $(BUILD_DIR)
	@ $(LD) $^ -o $(TARGET) $(LDFLAGS)

kxemu: $(TARGET)

run: $(TARGET)
	$(info + Running $(TARGET))
	@ $(TARGET) $(FLAGS)
