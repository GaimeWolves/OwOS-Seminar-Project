MAKE = make

submake_all_%: %
	$(MAKE) -C $< all
submake_setupiso_%: %
	$(MAKE) -C $< setupiso
	
all: submake_all_bootloader submake_all_os
setupiso: submake_setupiso_bootloader submake_setupiso_os
