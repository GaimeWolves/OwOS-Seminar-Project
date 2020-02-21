MAKE = make

submake_all_%: %
	$(MAKE) -C $< all
	
submake_clean_%: %
	$(MAKE) -C $< clean
	
all: submake_all_bootloader
clean: submake_clean_bootloader