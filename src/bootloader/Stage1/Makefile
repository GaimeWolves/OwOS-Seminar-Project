BUILD_DIR ?= "${PWD}/build"
BOOTLOADER_BUILD_DIR = $(BUILD_DIR)/bootloader/Stage1

NASM = nasm

.DEFAULT_GOAL := all
all: make-folders Stage1.bin
	@echo "Building Stage1 in\n$(BOOTLOADER_BUILD_DIR)"

setupboot:
	dd if=$(loopback_interface)p1 of=$(BOOTLOADER_BUILD_DIR)/Stage1.bin bs=1   seek=11 skip=11 count=79 conv=notrunc
	dd of=$(loopback_interface)p1 if=$(BOOTLOADER_BUILD_DIR)/Stage1.bin bs=512                 count=1
	dd of=$(loopback_interface)p1 if=$(BOOTLOADER_BUILD_DIR)/Stage1.bin bs=512 seek=2          count=1
	
Stage1.bin: main.asm
	$(NASM) -f bin $< -o "$(BOOTLOADER_BUILD_DIR)/$@" -I .
		
		
		
define make-folders
mkdir -p "$(BUILD_DIR)"; \
mkdir -p "$(BUILD_DIR)/bootloader"; \
mkdir -p "$(BUILD_DIR)/bootloader/Stage1";
endef

make-folders: 
	@$(call make-folders)

