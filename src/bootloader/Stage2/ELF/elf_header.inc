%include "./ELF/elf_structs.inc"

;-----------------------------------------
;	ProcessELFSection
;	Called with
;	EAX = Address of the file in memory
;	EBX = Address of the program section header in memory
;-----------------------------------------
ProcessELFSection:
	;Save ecx
	push ecx
	;Test if we can handle the section
	cmp DWORD[ebx + ELF_Program_Header_32.type], 0x01 ;(load)
	jne .return
	;Init memory segment
	mov edx, DWORD[ebx + ELF_Program_Header_32.mem_offset]
	mov ecx, DWORD[ebx + ELF_Program_Header_32.mem_size]
	.null_loop:
		mov BYTE[edx], 0x00
		;setup regs for the next repeat
		dec ecx
		inc edx
		cmp ecx, 0x00
		jne .null_loop
	;Copy data from file
	;SOURCE
	mov edx, DWORD[ebx + ELF_Program_Header_32.file_offset]
	add edx, eax
	mov esi, edx
	;DEST
	mov edx, DWORD[ebx + ELF_Program_Header_32.mem_offset]
	mov edi, edx
	;COUNT
	mov ecx, DWORD[ebx + ELF_Program_Header_32.file_size]

	;Copy bytes
	rep movsb

	.return:
		;Restore ecx
		pop ecx
		ret

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