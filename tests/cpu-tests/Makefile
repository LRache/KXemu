.PHONY: all run clean $(ALL)

RESULT = .result
$(shell > $(RESULT))

COLOR_RED   = \033[1;31m
COLOR_GREEN = \033[1;32m
COLOR_NONE  = \033[0m

define report-result
	@cat $(RESULT)
	@status=0; \
	if grep -qF "***FAIL***" $(RESULT); then status=1; fi; \
	rm -f $(RESULT); \
	exit $$status
endef

ALL = $(basename $(notdir $(shell find tests/. -name "*.c")))

all: $(addprefix Makefile., $(ALL))
	@echo "test list [$(words $(ALL)) item(s)]:" $(ALL)

$(ALL): %: Makefile.%

Makefile.%: tests/%.c latest
	@/bin/echo -e "INC_PATH += $(abspath ./include)\nSRCS += $<\nNAME=$*\ninclude ../abstract-machine/Makefile" > $@
	@if make -s -f $@ ISA=$(ISA) $(MAKECMDGOALS); then \
		printf "[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n" $* >> $(RESULT); \
	else \
		printf "[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n" $* >> $(RESULT); \
	fi
	-@rm -f Makefile.$*

run: all
	$(report-result)

debug: all
	$(report-result)

qemu: all
	$(report-result)

clean:
	rm -rf Makefile.* build/

latest:
