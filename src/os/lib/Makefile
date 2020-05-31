BUILD_DIR ?= "${PWD}/build"
LIB_BUILD_DIR := $(BUILD_DIR)/os/lib

C_SRC := $(shell find . -name '*.c') # Recursively finds all C-Files in this directory
C_OBJ := $(patsubst %.c, ${LIB_BUILD_DIR}/%.o, $(C_SRC)) # Replaces the file type of all C-Files with OBJ-Files
C_DIR := $(shell find . -name '*.c' -type f -exec dirname {} \; | uniq) # Gets directories of all C-Files

.DEFAULT_GOAL := all
all: make-folders ${LIB_BUILD_DIR}/lib.lib
	@echo "Building Kernel Library in:"
	@echo ${LIB_BUILD_DIR}

${LIB_BUILD_DIR}/lib.lib: ${C_OBJ}
	${AR} $@ $^

${LIB_BUILD_DIR}/%.o: %.c
	${CC} ${CFLAGS} -o $@ $<

make-folders:
	for dir in $(C_DIR); \
	do \
		mkdir -p ${LIB_BUILD_DIR}/$$dir; \
	done;
