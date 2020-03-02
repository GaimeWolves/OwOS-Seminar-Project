MAKE = make

export BUILD_DIR = $(PWD)/build
export SRC_DIR = $(PWD)/src

submake_all_%: %
	$(MAKE) -C $< all
	
all: submake_all_src
clean: 
	rm -rf build/
rebuild: clean all
