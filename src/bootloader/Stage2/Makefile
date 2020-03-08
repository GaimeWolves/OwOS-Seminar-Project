BUILD_DIR ?= "${PWD}/build"
BOOTLOADER_BUILD_DIR = $(BUILD_DIR)/bootloader/Stage2

NASM = nasm

.DEFAULT_GOAL := all
all: make-folders Stage2.sys
	@echo "Building Stage2 in\n$(BOOTLOADER_BUILD_DIR)"

setupiso:
	cp $(BOOTLOADER_BUILD_DIR)/Stage2.sys ${PWD}/fs/
	
Stage2.sys: main.asm
	$(NASM) -f bin $< -o "$(BOOTLOADER_BUILD_DIR)/$@" -I .
		
		
		
define make-folders
mkdir -p "$(BUILD_DIR)"; \
mkdir -p "$(BUILD_DIR)/bootloader"; \
mkdir -p "$(BUILD_DIR)/bootloader/Stage2";
endef

make-folders: 
	@$(call make-folders)

