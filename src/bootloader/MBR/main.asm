%define MBR_ADDRESS 0x0600

[bits 16]
[org MBR_ADDRESS]

cli
;push the drive number and pass it to the partition bootloader
push dx

%include "memcpy.inc"

jmp 0x0:main

main:
	
	
times 0x1B8-($-$$) db 0

times 0x1FE-($-$$) db 0
dw 0xAA55