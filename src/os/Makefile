BUILD_DIR ?= "${PWD}/build"
OS_BUILD_DIR := $(BUILD_DIR)/os

MAKE := make

export CC := i686-elf-gcc
export AR := ar rcs
export LFLAGS := -nostdlib -lgcc -fno-pie
export CFLAGS := -ffreestanding -c -O0 -masm=intel -g -Wall -Wextra -I$(SRC_DIR)/os/include -D _DEBUG

.DEFAULT_GOAL := all
all: make-folders ${OS_BUILD_DIR}/kernel.bin
	@echo "Building OS in:"
	@echo ${OS_BUILD_DIR}

submake_all_%: %
	-$(MAKE) -C $< all
	
setupiso: ${OS_BUILD_DIR}/stub.o
ifeq ($(wildcard $(OS_BUILD_DIR)/kernel.bin),)
	${CC} ${LFLAGS} -e entry -Ttext 0x100000 -Wl,--oformat=binary -o ${OS_BUILD_DIR}/kernel.bin ${OS_BUILD_DIR}/stub.o
endif
	cp ${OS_BUILD_DIR}/kernel.bin ${PWD}/fs/kernel.sys

${OS_BUILD_DIR}/kernel.bin: ${OS_BUILD_DIR}/entry.o submake_all_kernel submake_all_hal submake_all_stdlib
	${CC} ${LFLAGS} -e entry -Ttext 0x100000 -Wl,--oformat=binary -o $@ $< -L${OS_BUILD_DIR} -l:kernel/kernel.lib -l:hal/hal.lib -l:stdlib/stdlib.lib
	${CC} ${LFLAGS} -e entry -Ttext 0x100000 -o ${OS_BUILD_DIR}/kernel.elf $< -L${OS_BUILD_DIR} -l:kernel/kernel.lib -l:hal/hal.lib -l:stdlib/stdlib.lib

${OS_BUILD_DIR}/%.o: %.c
	-${CC} ${CFLAGS} -o $@ $<

make-folders:
	mkdir -p ${OS_BUILD_DIR}
