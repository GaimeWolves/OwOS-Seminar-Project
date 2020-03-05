MAKE = make

.DEFAULT_GOAL := all
all: submake_all_bootloader submake_all_os

submake_all_%: %
	-$(MAKE) -C $< all
submake_setupiso_%: %
	-$(MAKE) -C $< setupiso
submake_setupboot_%: %
	-$(MAKE) -C $< setupboot
	
setupiso: submake_setupiso_os
setupboot: submake_setupboot_bootloader