MAKE = make

export BUILD_DIR = $(PWD)/build

submake_all_%: %
	$(MAKE) -C $< all
	
all: submake_all_src
clean: 
	rm -rf build/