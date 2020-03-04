MAKE = make

export BUILD_DIR = "$(PWD)/build"
export SRC_DIR = "$(PWD)/src"

.DEFAULT_GOAL := all
all: setupiso submake_all_src submake_setupiso_src combineiso

submake_all_%: %
	-$(MAKE) -C $< all
submake_setupiso_%: %
	-$(MAKE) -C $< setupiso

setupiso:
	mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$(BUILD_DIR)/disk.iso count=100000
	dd if=/dev/zero of=$(BUILD_DIR)/partition.iso count=99999
	sfdisk $(BUILD_DIR)/disk.iso < disk.config

combineiso:
	dd if=$(BUILD_DIR)/partition.iso of=$(BUILD_DIR)/disk.iso seek=1 count=99999

clean:
	rm -rf build/
rebuild: clean all
