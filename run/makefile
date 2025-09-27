####################################################################
# Top-level Makefile for MINIREL with build/ output
####################################################################
include ../variables.mk

INCLUDE  = $(HOME)/include
ALGEBRA	 = $(HOME)/algebra
SCHEMA	 = $(HOME)/schema
PHYSICAL = $(HOME)/physical
PARSER   = $(HOME)/frontend

DFLAG =
CFLAGS = -g $(DFLAG) -I$(INCLUDE)

SRCS = main.c
OBJ = $(BUILD)/main.o
GLOB_OBJ = $(BUILD)/globals.o

ALGEBRA_OBJ = $(BUILD)/algebra/ALGEBRA.o
SCHEMA_OBJ = $(BUILD)/schema/SCHEMA.o
PHYSICAL_OBJ = $(BUILD)/physical/PHYSICAL.o
FES_OBJ = $(BUILD)/frontend/FES.o
FES_OBJ2 = $(BUILD)/frontend/fes.o

.PHONY: all clean algebra_build schema_build physical_build frontend_build

all: minirel

minirel: algebra_build schema_build physical_build frontend_build $(OBJ) $(GLOB_OBJ) | $(BUILD)
	@echo generating $@
	cc $(CFLAGS) $(ALGEBRA_OBJ) $(SCHEMA_OBJ) $(PHYSICAL_OBJ) $(FES_OBJ) $(OBJ) $(GLOB_OBJ) -o $(BUILD)/minirel

$(BUILD):
	mkdir -p $(BUILD)

# Compile globals.o into build/
$(GLOB_OBJ): $(HOME)/globals.c $(INCLUDE)/globals.h | $(BUILD)
	cc $(CFLAGS) -c $(HOME)/globals.c -o $@

# Compile main.o into build/
$(OBJ): $(SRCS) $(INCLUDE)/globals.h $(INCLUDE)/defs.h $(INCLUDE)/error.h | $(BUILD)
	cc $(CFLAGS) -c $(SRCS) -o $@

# Call sub-makefiles to build their objects
algebra_build:
	@$(MAKE) -C $(ALGEBRA)

schema_build:
	@$(MAKE) -C $(SCHEMA)

physical_build:
	@$(MAKE) -C $(PHYSICAL)

frontend_build:
	@$(MAKE) -C $(PARSER)

clean:
	rm -f $(BUILD)/*.o $(BUILD)/minirel
	@$(MAKE) -C $(ALGEBRA) clean
	@$(MAKE) -C $(SCHEMA) clean
	@$(MAKE) -C $(PHYSICAL) clean

