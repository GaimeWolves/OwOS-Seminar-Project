BUILD_DIR ?= "${PWD}/build"
STDLIB_BUILD_DIR := $(BUILD_DIR)/os
PREFIX ?= "ADD-PATH-HERE"

C_SRC := $(shell find . -name '*.c') # Recursively finds all C-Files in this directory
C_OBJ := $(patsubst %.c, ${STDLIB_BUILD_DIR}/stdlib/%.o, $(C_SRC)) # Replaces the file type of all C-Files with OBJ-Files
C_DIR := $(shell find . -name '*.c' -type f -exec dirname {} \; | uniq) # Gets directories of all C-Files
export LIBGCC_DIR := ${PREFIX}/lib/gcc/i686-elf/11.2.0/

.DEFAULT_GOAL := all
all: make-folders ${STDLIB_BUILD_DIR}/libc.so
	@echo "Building Standard Library in:"
	@echo ${STDLIB_BUILD_DIR}

${STDLIB_BUILD_DIR}/libc.so: ${C_OBJ}
	${LD} -shared ${LFLAGS} -o $@ $^ -L${STDLIB_BUILD_DIR} -lkernel -L${LIBGCC_DIR} -lgcc

${STDLIB_BUILD_DIR}/stdlib/%.o: %.c
	${CC} ${CFLAGS} ${KERNEL_STDLIB_INCLUDE} -fpic -o $@ $<

make-folders:
	mkdir -p ${STDLIB_BUILD_DIR}/stdlib/
	for dir in $(C_DIR); \
	do \
		mkdir -p ${STDLIB_BUILD_DIR}/stdlib/$$dir; \
	done;
