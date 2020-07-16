BUILD_DIR ?= "${PWD}/build"
LS_BUILD_DIR := $(BUILD_DIR)/os/ls

C_SRC := $(shell find . -name '*.c') # Recursively finds all C-Files in this directory
C_OBJ := $(patsubst %.c, ${LS_BUILD_DIR}/%.o, $(C_SRC)) # Replaces the file type of all C-Files with OBJ-Files
C_DIR := $(shell find . -name '*.c' -type f -exec dirname {} \; | uniq) # Gets directories of all C-Files

all: make-folders ${LS_BUILD_DIR}/ls.elf

${LS_BUILD_DIR}/%.o: %.c
	${CC} ${CFLAGS} ${KERNEL_STDLIB_INCLUDE} -fpic -o $@ $<

${LS_BUILD_DIR}/ls.elf: ${C_OBJ}
	${LD} ${LFLAGS} --entry=main --dynamic-linker=ld-owos -pie -o $@ $^ -L${BUILD_DIR}/os -lc -lkernel -L${LIBGCC_DIR} -lgcc

make-folders:
	mkdir -p ${LS_BUILD_DIR}/
	for dir in $(C_DIR); \
	do \
		mkdir -p ${LS_BUILD_DIR}/$$dir; \
	done;
