BUILD_DIR ?= "${PWD}/build"
BOOTLOADER_BUILD_DIR = $(BUILD_DIR)/bootloader/MBR

NASM = nasm

.DEFAULT_GOAL := all
all: make-folders MBR.bin
	@echo "Building MBR in\n$(BOOTLOADER_BUILD_DIR)"

setupboot:
	dd if=$(loopback_interface) of=$(BOOTLOADER_BUILD_DIR)/MBR.bin bs=1		seek=440	skip=440	count=70	conv=notrunc
	dd of=$(loopback_interface) if=$(BOOTLOADER_BUILD_DIR)/MBR.bin bs=512							count=1
	
MBR.bin: main.asm
		$(NASM) -f bin $< -o "$(BOOTLOADER_BUILD_DIR)/$@" -I .
		
		
		
define make-folders
mkdir -p "$(BUILD_DIR)"; \
mkdir -p "$(BUILD_DIR)/bootloader"; \
mkdir -p "$(BUILD_DIR)/bootloader/MBR";
endef

make-folders: 
	@$(call make-folders)
