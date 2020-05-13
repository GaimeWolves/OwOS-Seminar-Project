BUILD_DIR ?= "${PWD}/build"
HAL_BUILD_DIR := $(BUILD_DIR)/os/hal

C_SRC := $(shell find . -name '*.c') # Recursively finds all C-Files in this directory
C_OBJ := $(patsubst %.c, ${HAL_BUILD_DIR}/%.o, $(C_SRC)) # Replaces the file type of all C-Files with OBJ-Files
C_DIR := $(shell find . -name '*.c' -type f -exec dirname {} \; | uniq) # Gets directories of all C-Files

.DEFAULT_GOAL := all
all: make-folders ${HAL_BUILD_DIR}/hal.lib
	@echo "Building Hardware Abstraction Layer in:"
	@echo ${HAL_BUILD_DIR}

${HAL_BUILD_DIR}/hal.lib: ${C_OBJ}
	${AR} $@ $^

${HAL_BUILD_DIR}/%.o: %.c
	${CC} ${CFLAGS} -mgeneral-regs-only -o $@ $<

make-folders:
	for dir in $(C_DIR); \
	do \
		mkdir -p ${HAL_BUILD_DIR}/$$dir; \
	done;
