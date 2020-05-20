%include "./ELF/elf_structs.inc"

;-----------------------------------------
;	ProcessELFSection
;	Called with
;	EAX = Address of the file in memory
;	EBX = Address of the program section header in memory
;-----------------------------------------
ProcessELFSection:
	

;-----------------------------------------
;	SetupELFSections
;	Called with
;	EAX = Address of the file in memory
;	EAX => Entry point
;-----------------------------------------
SetupELFSections:
	;Check for sanity of the header
	cmp DWORD[eax + ELF_Header.magic_number], 0x464C457f
	jne .error
	cmp BYTE[eax + ELF_Header.architecture], 0x01
	jne .error
	cmp BYTE[eax + ELF_Header.byte_type], 0x01
	jne .error
	cmp WORD[eax + ELF_Header.type], 0x02
	jne .error

	;Parse program header
	;Get address of program header
	mov ebx, DWORD[eax + ELF_Header.ph_pos]
	add ebx, eax
	;Get count of program header entries
	xor ecx, ecx
	mov cx, WORD[eax + ELF_Header.ph_entry_count]
	;Parse loop
	.loop:
		;Pass to sub routine
		call ProcessELFSection

		;Setup next loop registers
		dec ecx		;decrease counter
		add bx, WORD[eax + ELF_Header.ph_entry_size]	;next program header

		;Jump to next loop if not finished
		cmp ecx, 0x00
		jne .loop

	;Return entry point
	mov eax, DWORD[eax + ELF_Header.entry_point]
	ret
	.error:
		cli
		hlt