MAKE = make

.DEFAULT_GOAL := all
all: submake_all_Stage1 submake_all_MBR

submake_all_%: %
	-$(MAKE) -C $< all
submake_setupiso_%: %
	-$(MAKE) -C $< setupiso
	
setupiso: submake_setupiso_Stage1 submake_setupiso_MBR