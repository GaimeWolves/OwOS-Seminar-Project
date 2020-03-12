struc GDT_descriptor
	.LimitLow:			resw 1
	.BaseLow:			resw 1
	.BaseMedium:		resb 1
	.AccessByte:		resb 1
	.LimitHigh_Flags:	resb 1
	.BaseHigh:			resb 1
endstruc

GDT_Start:
;-----------------------------------------
	;Null descriptor
	istruc GDT_descriptor
	iend
;-----------------------------------------
	;Code descriptor
	istruc GDT_descriptor
		at GDT_descriptor.LimitLow,			dw 0xFFFF
		at GDT_descriptor.BaseLow,			dw 0x0000
		at GDT_descriptor.BaseMedium,		db 0x00
		at GDT_descriptor.AccessByte,		db 0b10011010
		at GDT_descriptor.LimitHigh_Flags,	db 0b11001111
		at GDT_descriptor.BaseHigh,			db 0x00
	iend
;-----------------------------------------
	;Data descriptor
	istruc GDT_descriptor
		at GDT_descriptor.LimitLow,			dw 0xFFFF
		at GDT_descriptor.BaseLow,			dw 0x0000
		at GDT_descriptor.BaseMedium,		db 0x00
		at GDT_descriptor.AccessByte,		db 0b10010010
		at GDT_descriptor.LimitHigh_Flags,	db 0b11001111
		at GDT_descriptor.BaseHigh,			db 0x00
	iend
;-----------------------------------------
GDT_End:

GDTR:
	dw GDT_End - GDT_Start - 1		;Size of the GDT
	dd GDT_Start					;Address of GDT in memory