;-----------------------------------------
;	GetMMap
;	Called with
;	ES:DI = Memory buffer
;	ES:DI => End of Memory buffer
;-----------------------------------------
GetMMap:
	;Make space for the size entry
	add di, 0x04
	;Clear EBX and EAX
	xor ebx, ebx
	.loadLoop:
		xor eax, eax
		;EDX gets a magic number
		mov edx, 0x534D4150
		;Set interrupt subroutine
		mov ax, 0xE820
		;Set cx to 24
		mov cx, 0x18
		;Call interrupt
		int 0x15
		;Test if we reached the end
		jc .end
		cmp ebx, 0x00
		je .end
		;Save returned size
		xor eax, eax
		mov al, cl
		mov DWORD[es:di-4], eax
		;Increase the offset
		add di, ax
		add di, 0x04
		jmp .loadLoop

	.end:		
		;Save returned size
		xor eax, eax
		mov al, cl
		mov DWORD[es:di-4], eax
		;Increase the offset
		add di, ax
		
		ret