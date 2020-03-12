struc Multiboot
	.flags:				resb 4	;required
	.mem_lower:			resb 4	;flags[0]
	.mem_upper:			resb 4	;flags[0]
	.boot_device:		resb 4	;flags[1]
	.cmdline:			resb 4	;flags[2]
	.mods_count:		resb 4	;flags[3]
	.mods_addr:			resb 4	;flags[3]
	.syms:				resb 16	;flags[4] or flags[5]
	.mmap_length:		resb 4	;flags[6]
	.mmap_addr:			resb 4	;flags[6]
	.drives_length:		resb 4	;flags[7]
	.drives_addr:		resb 4	;flags[7]
	.config_table:		resb 4	;flags[8]
	.boot_loader_name:	resb 4	;flags[9]
	.apm_table:			resb 4	;flags[10]
	.vbe_control_info:	resb 4	;flags[11]
	.vbe_mode_info:		resb 4	;flags[11]
	.vbe_mode:			resb 2	;flags[11]
	.vbe_interface_seq:	resb 2	;flags[11]
	.vbe_interface_off:	resb 2	;flags[11]
	.vbe_interface_len:	resb 2	;flags[11]
	.framebuffer_addr:	resb 8	;flags[12]
	.framebuffer_pitch:	resb 4	;flags[12]
	.framebuffer_width:	resb 4	;flags[12]
	.framebuffer_height:resb 4	;flags[12]
	.framebuffer_bpp:	resb 1	;flags[12]
	.framebuffer_type:	resb 1	;flags[12]
	.color_info:		resb 6	;flags[12]
endstruc

struc boot_device
	.drive:				resb 1	;drive number
	.part1:				resb 1	;top-level partition
	.part2:				resb 1	;sub-partition
	.part3:				resb 1
endstruc