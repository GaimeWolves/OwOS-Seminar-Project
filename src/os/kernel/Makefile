BUILD_DIR ?= "${PWD}/build"
KERNEL_BUILD_DIR := $(BUILD_DIR)/os/kernel

C_SRC := $(shell find . -name '*.c') # Recursively finds all C-Files in this directory
C_OBJ := $(patsubst %.c, ${KERNEL_BUILD_DIR}/%.o, $(C_SRC)) # Replaces the file type of all C-Files with OBJ-Files
C_DIR := $(shell find . -name '*.c' -type f -exec dirname {} \; | uniq) # Gets directories of all C-Files

.DEFAULT_GOAL := all
all: make-folders ${KERNEL_BUILD_DIR}/kernel.lib
	@echo "Building Kernel in:"
	@echo ${KERNEL_BUILD_DIR}

${KERNEL_BUILD_DIR}/kernel.lib: ${C_OBJ}
	${AR} $@ $^

${KERNEL_BUILD_DIR}/%.o: %.c
	${CC} ${CFLAGS} ${KERNEL_INCLUDE} -o $@ $<

make-folders:
	for dir in $(C_DIR); \
	do \
		mkdir -p ${KERNEL_BUILD_DIR}/$$dir; \
	done;
