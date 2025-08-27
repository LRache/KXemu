INSTPAT_SRC_DIR = ./src/cpu/riscv/instpat
INSTPAT_DST_DIR = ./src/cpu/riscv/autogen
INSTPAT_NAMES = base compressed

INSTPAT_SRC_FILES = $(addprefix $(INSTPAT_SRC_DIR)/, $(addsuffix .instpat, $(INSTPAT_NAMES)))

INSTPAT_DST_FILES = $(addprefix $(INSTPAT_DST_DIR)/, $(addsuffix -decoder.inc, $(INSTPAT_NAMES)))

DECODE_GEN_SCRIPT = ./scripts/riscv-decoder-gen/main.py

$(INSTPAT_DST_DIR)/%-decoder.inc: $(INSTPAT_SRC_DIR)/%.instpat $(DECODE_GEN_SCRIPT)
	$(info + GEN $@)
	@ mkdir -p $(INSTPAT_DST_DIR)
	@ python3 $(DECODE_GEN_SCRIPT) --input $< --output $@

instpat: $(INSTPAT_DST_FILES)

kxemu: instpat
