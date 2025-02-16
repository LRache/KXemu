INSTPAT_SRC_DIR = ./src/cpu/riscv/instpat
INSTPAT_DST_DIR = ./src/cpu/riscv/autogen
INSTPAT_NAMES = base compressed

INSTPAT_SRC_FILES = $(addprefix $(INSTPAT_SRC_DIR)/, $(addsuffix .instpat, $(INSTPAT_NAMES)))

INSTPAT_DST_FILES = $(addprefix $(INSTPAT_DST_DIR)/, $(addsuffix -decoder.h, $(INSTPAT_NAMES)))

DECODE_GEN_SCRIPT = ./scripts/decoder-gen/main.py

$(INSTPAT_DST_DIR)/%-decoder.h: $(INSTPAT_SRC_DIR)/%.instpat
	$(info + GEN $@)
	@ mkdir -p $(INSTPAT_DST_DIR)
	@ python3 $(DECODE_GEN_SCRIPT) --input $< --output $@ --prefix "this->do_"

instpat: $(INSTPAT_DST_FILES)

kxemu: instpat
