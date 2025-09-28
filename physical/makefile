####################################################################
# Makefile for PHYSICAL layer with build/physical output
####################################################################
include ../variables.mk

INCLUDE = $(HOME)/include
BUILD_DIR = $(BUILD)/physical

DFLAG =
CFLAGS = -g $(DFLAG) -I$(INCLUDE)

SRCS = \
	error.c createcats.c opencats.c closecats.c openrel.c closerel.c findrelnum.c \
	readpage.c flushpage.c writerec.c findrec.c getnextrec.c insertrec.c deleterec.c

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(BUILD_DIR)/PHYSICAL.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(INCLUDE)/globals.h $(INCLUDE)/defs.h $(INCLUDE)/error.h | $(BUILD_DIR)
	cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/PHYSICAL.o: $(OBJ)
	@echo generating $@
	@ld -r $(OBJ) -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/PHYSICAL.o

