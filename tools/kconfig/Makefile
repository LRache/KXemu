NAME = conf
obj := build
SRCS += confdata.c expr.c preprocess.c symbol.c util.c
SRCS += $(obj)/lexer.lex.c $(obj)/parser.tab.c
CC = gcc
LD = g++
CFLAGS += -DYYDEBUG
INC_PATH += .
DISTRO = $(shell cat /etc/os-release | grep PRETTY_NAME | sed 's/PRETTY_NAME=//')

WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

INC_PATH := $(WORK_DIR)/include $(INC_PATH)
OBJ_DIR  = $(BUILD_DIR)/obj-$(NAME)$(SO)
BINARY   = $(BUILD_DIR)/$(NAME)$(SO)

ifeq ($(DISTRO),"Gentoo Linux")
LIBS += -ltinfo
endif

ifeq ($(NAME),conf)
SRCS += conf.c
else ifeq ($(NAME),mconf)
SRCS += mconf.c $(shell find lxdialog/ -name "*.c")
LIBS += -lncurses
else
$(error bad target=$(NAME))
endif

$(obj)/lexer.lex.o: $(obj)/parser.tab.h
$(obj)/lexer.lex.c: lexer.l $(obj)/parser.tab.h
	@echo + LEX $@
	@flex -o $@ $<

$(obj)/parser.tab.c $(obj)/parser.tab.h: parser.y
	@echo + YACC $@
	@bison -v $< --defines=$(obj)/parser.tab.h -o $(obj)/parser.tab.c

conf:
	@$(MAKE) -s

mconf:
	@$(MAKE) -s NAME=mconf

# Compilation flags
INCLUDES = $(addprefix -I, $(INC_PATH))
CFLAGS  := -O2 -MMD -Wall -Werror $(INCLUDES) $(CFLAGS)
LDFLAGS := -O2 $(LDFLAGS)

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o) $(CXXSRC:%.cc=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<
	$(call call_fixdep, $(@:.o=.d), $@)

$(OBJ_DIR)/%.o: %.cc
	@echo + CXX $<
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<
	$(call call_fixdep, $(@:.o=.d), $@)

# Depencies
-include $(OBJS:.o=.d)

# Some convenient rules

app: $(BINARY)

$(BINARY):: $(OBJS) $(ARCHIVES)
	@echo + LD $@
	@$(LD) -o $@ $(OBJS) $(LDFLAGS) $(ARCHIVES) $(LIBS)

clean:
	-rm -rf $(BUILD_DIR)

.PHONY: conf mconf

.DEFAULT_GOAL := app
