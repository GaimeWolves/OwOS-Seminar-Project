;-----------------------------------------
;	GetUpperMem
;	Called with
;	EAX => Memory above 1MB
;-----------------------------------------
GetUpperMem:
	;We need to test if the result is written to eax/ebx or ecx/edx
	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	;Call Bios function
	mov ax, 0xe801
	int 0x15
	jc .error
	cmp	ah, 0x86		;unsupported function
	je .error
	cmp	ah, 0x80		;invalid command
	je .error
	;Test which registers are used
	cmp ecx, 0
	je .eax_used
	;If not fix it
	mov eax, ecx
	mov ebx, edx

	.eax_used:
		;Save the value from 1MiB - 16MiB
		push eax
		;Above 16MiB are 64KiB blocks
		mov eax, ebx
		mov ebx, 64
		mul ebx
		;Add the 1MiB - 16MiB value
		pop ebx
		add eax, ebx
		;count of 1KiB blocks in eax
		shl eax, 10

	ret

	.error:
		cli
		hlt