;-----------------------------------------
;	a20EnabledTest
;	ax => 1 = enabled, 0 = disabled
;-----------------------------------------
a20EnabledTest:
	;Check if line A20 is enabled
	;Save memory address registers
	push ds
	push es
	push si
	push di

	;No interrupts please
	cli

	;If line A20 is not enabled 0x0000:0x7DFE and 0xFFFF:0x7E0E point to the same memory
	;because of a overflow in memory address lines
	xor ax, ax
	mov es, ax

	not ax
	mov ds, ax

	mov di, 0x7dfe
	mov si, 0x7e0e

	;Test if they are the same
	mov ax, WORD[es:di]
	mov bx, WORD[ds:si]
	cmp ax, bx ;not the same means already enabled
	jne .a20enabled

	;If they are the same it could be coincidence
	mov WORD[es:di], 0xA5A5
	cmp WORD[ds:si], 0xA5A5
	jne .a20enabled ;not the same means diverent memory locations so already enabled
	jmp .a20notEnabled ;the same => not enabled

	.end:
	;Restore memory address registers
	pop di
	pop si
	pop es
	pop ds
	;Restore interrupts
	sti
	;return
	ret

	.a20notEnabled:
		mov ax, 0
		jmp .end
	.a20enabled:
		mov ax, 1
		jmp .end

;-----------------------------------------
;	a20KeyboardControllerCommand
;-----------------------------------------
a20KeyboardControllerCommand:
	;Try enabling the A20 line
	;through sending 0xDD command to the keyboard controller
	mov al, 0xdd ;Load command to al
	out 0x64, al ;Send to 0x64 (keyboard controller command register)
	ret