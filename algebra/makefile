####################################################################
# Makefile for ALGEBRA layer with build/algebra output
####################################################################
include ../variables.mk

INCLUDE = $(HOME)/include
BUILD_DIR = $(BUILD)/algebra

DFLAG =
CFLAGS = -g $(DFLAG) -I$(INCLUDE)

SRCS = insert.c delete.c join.c project.c select.c 

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(BUILD_DIR)/ALGEBRA.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(INCLUDE)/globals.h $(INCLUDE)/defs.h $(INCLUDE)/error.h | $(BUILD_DIR)
	cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ALGEBRA.o: $(OBJ)
	@echo generating $@
	@ld -r $(OBJ) -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/ALGEBRA.o

