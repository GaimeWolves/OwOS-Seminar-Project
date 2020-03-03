BOOTLOADER_BUILD_DIR = $(BUILD_DIR)/Bootloader/Stage1

NASM = nasm

all: make-folders Stage1.bin
	@echo "Building Stage1 in\n$(BOOTLOADER_BUILD_DIR)"

setupiso:
	
Stage1.bin: main.asm
		$(NASM) -f bin $< -o "$(BOOTLOADER_BUILD_DIR)/$@" -I .
		
		
		
define make-folders
mkdir -p "$(BUILD_DIR)"; \
mkdir -p "$(BUILD_DIR)/Bootloader"; \
mkdir -p "$(BUILD_DIR)/Bootloader/Stage1";
endef

make-folders: 
	@$(call make-folders)
