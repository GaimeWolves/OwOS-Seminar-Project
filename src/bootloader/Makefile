MAKE = make

submake_all_%: %
	$(MAKE) -C $< all
submake_setupiso_%: %
	$(MAKE) -C $< setupiso
	
all: submake_all_Stage1 submake_all_MBR
setupiso: submake_setupiso_Stage1 submake_setupiso_MBR