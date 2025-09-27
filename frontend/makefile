INCLUDE = .

BUILD_DIR = ../build/frontend

DFLAG = 
CFLAGS = -g $(DFLAG) -I$(INCLUDE)

SRCS = fes.c
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(BUILD_DIR)/FES.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/fes.o: fes.c $(INCLUDE)/fes.h | $(BUILD_DIR)
	cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/FES.o: $(OBJ)
	@echo generating $@
	@ld -r $(OBJ) -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/FES.o

