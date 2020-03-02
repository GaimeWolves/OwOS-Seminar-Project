OS_BUILD_DIR = $(BUILD_DIR)/os

MAKE = make

export CC := i686-elf-gcc
export AR := ar rcs
export LFLAGS := -ffreestanding -fno-pie -nostdlib
export CFLAGS := -c -O0 -m32 -masm=intel -g -Wall -Wextra -I$(SRC_DIR)/os/include -D _DEBUG -D _I686

submake_all_%: %
	$(MAKE) -C $< all
	
all: make-folders ${OS_BUILD_DIR}/kernel.elf
	@echo "Building OS in:"
	@echo ${OS_BUILD_DIR}

${OS_BUILD_DIR}/kernel.elf: ${OS_BUILD_DIR}/entry.o submake_all_kernel submake_all_hal submake_all_stdlib
	${CC} ${LFLAGS} -e entry -Ttext 0x100000 -o $@ $< -L${OS_BUILD_DIR} -l:kernel/kernel.lib -l:hal/hal.lib -l:stdlib/stdlib.lib -lgcc

${OS_BUILD_DIR}/entry.o: entry.c
	${CC} ${CFLAGS} -o $@ $<

make-folders:
	mkdir -p ${OS_BUILD_DIR}
