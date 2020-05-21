struc ELF_Header
	.magic_number:		resb 4	;elf magic number 0x7c then "ELF" ascii
	.architecture:		resb 1	;32 bit / 64 bit
	.byte_type:			resb 1	;little endian, big endian
	.header_version:	resb 1	;ELF header version
	.os_abi:			resb 1	;normaly 0 for System V
						resb 8	;unused
	.type:				resb 2	;1 = relocatable, 2 = executable, 3 = shared, 4 = core
	.is:				resb 2	;instruction set
	.version:			resb 4	;ELF version
	.entry_point:		resb 4	;program entry point
	.ph_pos:			resb 4	;program header table position
	.sh_pos:			resb 4	;section header table position
	.flags:				resb 4	;Architecture dependant (undefined for i386)
	.header_size:		resb 2	;Size of this header
	.ph_entry_size:		resb 2	;Size of an entry in the program header table
	.ph_entry_count:	resb 2	;Number of entries in the program header table
	.sh_entry_size:		resb 2	;Size of an entry in the section header table
	.sh_entry_count:	resb 2	;Number of entries in the section header table
	.sh_index:			resb 2	;Index in section header table with the section names
endstruc

struc ELF_Program_Header_32
	.type:				resb 4	;Type of segment
	.file_offset:		resb 4	;Segment offset in file
	.mem_offset:		resb 4	;Address in memory for this segment
						resb 4	;Unused
	.file_size:			resb 4	;Segment size in file
	.mem_size:			resb 4	;Size in memory for this segment
	.flags:				resb 4	;1 = executable, 2 = writable, 4 = readable
	.align:				resb 4	;required memory alignment
endstruc