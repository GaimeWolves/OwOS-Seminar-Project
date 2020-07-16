MAKE = make

export PWD = $(shell pwd)

export BUILD_DIR = "$(PWD)/build"
export SRC_DIR = "$(PWD)/src"

export loopback_interface := $(shell losetup -f)

.DEFAULT_GOAL := all
all: setupiso submake_all_src submake_setupboot_src setup_iso_settings submake_setupiso_src clear_iso_settings

submake_all_%: %
	-$(MAKE) -C $< all
submake_setupiso_%: %
	-$(MAKE) -C $< setupiso
submake_setupboot_%: %
	-$(MAKE) -C $< setupboot

setupiso:
	mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$(BUILD_DIR)/disk.iso count=100000
	sfdisk $(BUILD_DIR)/disk.iso < disk.config
ifndef loopback_interface
	$(error no loopback interface available)
endif
	mkdir -p fs
	losetup -P $(loopback_interface) $(BUILD_DIR)/disk.iso
	mkfs.vfat -F 32 -D 0x80 -b 2 $(loopback_interface)p1

setup_iso_settings:
	mount $(loopback_interface)p1 fs

clear_iso_settings:
	mkdir -p fs/bin

	-for dir in `find build/os/* -type d`; \
	do \
		for elf in $$dir/*.elf; \
		do \
			if [ -f $$elf ]; then \
				cp $$elf fs/bin/; \
			fi; \
		done; \
	done;

	umount fs
	losetup -d $(loopback_interface)

clean:
	rm -rf $(BUILD_DIR)
rebuild: clean all
