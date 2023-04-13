SRC_DIR ?= "${PWD}/src"
BUILD_DIR ?= "${PWD}/build"
OS_BUILD_DIR := $(BUILD_DIR)/os
PREFIX ?= "ADD-PATH-HERE"

MAKE := make

export LD := ${PREFIX}/bin/i686-elf-ld
export CC := ${PREFIX}/bin/i686-elf-gcc
export AR := ar rcs
export KERNEL_LINKER_SCRIPT := -T ${SRC_DIR}/os/linker.ld
export KERNEL_LIB_LINKER_SCRIPT := -T ${SRC_DIR}/os/linker-so.ld
export LFLAGS := -nostdlib
export KERNEL_INCLUDE := -I$(SRC_DIR)/os/include
export KERNEL_STDLIB_INCLUDE := -I$(SRC_DIR)/os/stdlib/include
export CFLAGS := -ffreestanding -c -O0 -g -Wall -Wextra -D _DEBUG

.DEFAULT_GOAL := all
all: make-folders ${OS_BUILD_DIR}/kernel.elf submake_all_stdlib submake_all_applications
	@echo "Building OS in:"
	@echo ${OS_BUILD_DIR}

submake_all_%: %
	-$(MAKE) -C $< all
	
setupiso: ${OS_BUILD_DIR}/entry.o ${OS_BUILD_DIR}/stub.o
ifneq ($(wildcard $(OS_BUILD_DIR)/kernel.elf),)
	${CC} ${LFLAGS} -o ${OS_BUILD_DIR}/kernel.elf $^ -lgcc
endif
	cp ${OS_BUILD_DIR}/kernel.elf ${PWD}/fs/kernel.elf
	cp ${OS_BUILD_DIR}/libc.so ${PWD}/fs/libc.so
	cp ${OS_BUILD_DIR}/libkernel.so ${PWD}/fs/libkernel.so

${OS_BUILD_DIR}/kernel.elf: ${OS_BUILD_DIR}/entry.o submake_all_kernel submake_all_hal submake_all_lib
	${CC} ${LFLAGS} -fno-pie ${KERNEL_LINKER_SCRIPT} -o ${OS_BUILD_DIR}/kernel.elf $< -L${OS_BUILD_DIR} -l:kernel/kernel.lib -l:hal/hal.lib -l:lib/lib.lib -lgcc
	${CC} -Wl,-r ${LFLAGS} -fno-pie ${KERNEL_LINKER_SCRIPT} -o ${OS_BUILD_DIR}/kernel-r.elf $< -L${OS_BUILD_DIR} -l:kernel/kernel.lib -l:hal/hal.lib -l:lib/lib.lib -lgcc
	${LD} -shared ${LFLAGS} ${KERNEL_LIB_LINKER_SCRIPT} -o ${OS_BUILD_DIR}/libkernel.so -L${OS_BUILD_DIR} -l:kernel-r.elf

${OS_BUILD_DIR}/%.o: %.c
	-${CC} ${CFLAGS} ${KERNEL_INCLUDE} -o $@ $<

${OS_BUILD_DIR}/%.o: %.S
	-${CC} ${CFLAGS} ${KERNEL_INCLUDE} -o $@ $<
	
make-folders:
	mkdir -p ${OS_BUILD_DIR}