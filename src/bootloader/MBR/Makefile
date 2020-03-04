BUILD_DIR ?= "${PWD}/build"
BOOTLOADER_BUILD_DIR = $(BUILD_DIR)/Bootloader/MBR

NASM = nasm

.DEFAULT_GOAL := all
all: make-folders MBR.bin
	@echo "Building MBR in\n$(BOOTLOADER_BUILD_DIR)"

setupiso:
	
MBR.bin: main.asm
		$(NASM) -f bin $< -o "$(BOOTLOADER_BUILD_DIR)/$@" -I .
		
		
		
define make-folders
mkdir -p "$(BUILD_DIR)"; \
mkdir -p "$(BUILD_DIR)/Bootloader"; \
mkdir -p "$(BUILD_DIR)/Bootloader/MBR";
endef

make-folders: 
	@$(call make-folders)
