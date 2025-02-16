BUILD_DIR = ./export/build/$(ISA)

LDFLAGS += -nostdlib -shared -fPIC

EXPORT_DIR := export

$(EXPORT_DIR)/$(ISA)/$(ISA)-kxemu.so: $(OBJS)
	$(info + LD $@)
	@ mkdir -p $(EXPORT_DIR)/$(ISA)
	@ $(LD) $(OBJS) -o $@ $(LDFLAGS) $(CXXFLAGS)

$(EXPORT_DIR)/$(ISA)/$(ISA)-kxemu.a: $(OBJS)
	$(info + AR $@)
	@ mkdir -p $(EXPORT_DIR)/$(ISA)
	@ $(AR) rcs $@ $(OBJS)

export: $(EXPORT_DIR)/$(ISA)/$(ISA)-kxemu.so $(EXPORT_DIR)/$(ISA)/$(ISA)-kxemu.a
	$(info + Exporting Headers $(ISA))
	@ python3 ./scripts/export/export.py

.PHONY: export
