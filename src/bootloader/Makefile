BUILD_DIR ?= "${PWD}/build"

MAKE = make

.DEFAULT_GOAL := all
all: submake_all_Stage2 submake_all_Stage1 submake_all_MBR

submake_all_%: %
	-$(MAKE) -C $< all
submake_setupboot_%: %
	-$(MAKE) -C $< setupboot
submake_setupiso_%: %
	-$(MAKE) -C $< setupiso
	
setupboot: submake_setupboot_Stage1 submake_setupboot_MBR
setupiso: submake_setupiso_Stage2
