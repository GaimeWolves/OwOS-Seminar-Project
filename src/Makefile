MAKE = make

submake_all_%: %
	$(MAKE) -C $< all
	
all: submake_all_bootloader submake_all_os
