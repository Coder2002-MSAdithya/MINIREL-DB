####################################################################
# Makefile for SCHEMA layer with build/schema output
####################################################################
include ../variables.mk

INCLUDE = $(HOME)/include
BUILD_DIR = $(BUILD)/schema

DFLAG =
CFLAGS = -g $(DFLAG) -I$(INCLUDE)

SRCS = \
	createdb.c opendb.c closedb.c destroydb.c quit.c \
	create.c createfromattrlist.c destroy.c load.c print.c sort.c buildindex.c dropindex.c

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(BUILD_DIR)/SCHEMA.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(INCLUDE)/globals.h $(INCLUDE)/defs.h $(INCLUDE)/error.h | $(BUILD_DIR)
	cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/SCHEMA.o: $(OBJ)
	@echo generating $@
	@ld -r $(OBJ) -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/SCHEMA.o

