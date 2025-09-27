####################################################################
#
#	This is the Makefile for creating MINIREL.
#
####################################################################

# where to look for the files

HOME = ..
INCLUDE  = $(HOME)/include
ALGEBRA	 = $(HOME)/algebra
SCHEMA	 = $(HOME)/schema
PHYSICAL = $(HOME)/physical
PARSER   = $(HOME)/frontend

DFLAG =
CFLAGS = -g $(DFLAG) -I$(INCLUDE)

# Source and object files
SRCS = main.c
OBJ = main.o
GLOB_OBJ = $(HOME)/globals.o

ALGEBRA_OBJ = $(ALGEBRA)/ALGEBRA.o
SCHEMA_OBJ = $(SCHEMA)/SCHEMA.o
PHYSICAL_OBJ = $(PHYSICAL)/PHYSICAL.o
FES_OBJ = $(PARSER)/FES.o

#############################################################
#  Targets
#############################################################

proj: minirel

minirel: $(ALGEBRA_OBJ) $(SCHEMA_OBJ) $(PHYSICAL_OBJ) $(FES_OBJ) $(OBJ) $(GLOB_OBJ)
	@echo generating $@
	cc $(CFLAGS) $^ -o $@

# Compile globals.o from the root MINIREL folder
$(GLOB_OBJ): $(HOME)/globals.c $(INCLUDE)/globals.h
	cc $(CFLAGS) -c $(HOME)/globals.c -o $@

# Compile main.o from current directory
$(OBJ): $(SRCS) $(INCLUDE)/globals.h $(INCLUDE)/defs.h $(INCLUDE)/error.h
	cc $(CFLAGS) -c $(SRCS) -o $@

# Delegate building the following object files to their respective folders
$(ALGEBRA_OBJ):
	@$(MAKE) -C $(ALGEBRA)

$(SCHEMA_OBJ):
	@$(MAKE) -C $(SCHEMA)

$(PHYSICAL_OBJ):
	@$(MAKE) -C $(PHYSICAL)

$(FES_OBJ):
	@$(MAKE) -C $(PARSER)

# Clean up only current directory
clean:
	rm -f *.o minirel




