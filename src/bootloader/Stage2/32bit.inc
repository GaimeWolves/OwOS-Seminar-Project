setGDT:
	;Load the GDT and switch to 32 bits
	cli			;Stop the interrupts until we entered kernel
	lgdt [GDTR]	;Load GDT descriptors
	
	;Switch to protected mode
	mov eax, cr0	;Get the processor control register
	or eax, 0x0001	;bit 0 controlls protected mode
	mov cr0, eax	;Save the control register back

	;Get Stage2 args back
	pop dx
	pop ax		;Get ds 
	pop bx		;Get si
	shl ax, 4	;Shift the ds value to its logical value
	add ax, bx	;Logical address of partition entry in ax
	
	;Far jump to 32 bits code
	jmp 0x08:setup32
