%define MBR_ADDRESS 0x9000

[bits 16]
[org MBR_ADDRESS]

;push the drive number and pass it to the partition bootloader
push dx

%include "memcpy.inc"

jmp main

main:
	
	
times 0x1B8-($-$$) db 0

times 0x1FE-($-$$) db 0
dw 0xAA55