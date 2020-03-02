STDLIB_BUILD_DIR = $(BUILD_DIR)/os/stdlib

C_SRC := $(shell find . -name '*.c') # Recursively finds all C-Files in this directory
C_OBJ := $(patsubst %.c, ${STDLIB_BUILD_DIR}/%.o, $(C_SRC)) # Replaces the file type of all C-Files with OBJ-Files
C_DIR := $(shell find . -name '*.c' -type f -exec dirname {} \; | uniq) # Gets directories of all C-Files

all: make-folders ${STDLIB_BUILD_DIR}/stdlib.lib
	@echo "Building Standard Library in:"
	@echo ${STDLIB_BUILD_DIR}

${STDLIB_BUILD_DIR}/stdlib.lib: ${C_OBJ}
	${AR} $@ $^

${STDLIB_BUILD_DIR}/%.o: %.c
	${CC} ${CFLAGS} -o $@ $<

make-folders:
	for dir in $(C_DIR); \
	do \
		mkdir -p ${STDLIB_BUILD_DIR}/$$dir; \
	done;
