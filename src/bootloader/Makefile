MAKE = make

submake_all_%: %
	$(MAKE) -C $< all
	
all: submake_all_Stage1 submake_all_MBR