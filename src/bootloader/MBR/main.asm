;-----------------------------------------
;	Constants
;-----------------------------------------
%define MBR_ADDRESS 0x0600
%define STACK_POSITION 0xFFFF

;-----------------------------------------
;	Structs
;-----------------------------------------
%include "../Help_Functions/MBR.inc"
%include "../Help_Functions/struct.inc"

;-----------------------------------------
;	Start
;-----------------------------------------
[bits 16]
[org MBR_ADDRESS]

;Interrupts are evil right now
cli

;Setup stack pointer
mov sp, STACK_POSITION
;push the drive number and pass it to the partition bootloader
push dx

;relocate ourself to 0x0600
%include "memcpy.inc"

;far jump to the main programme
jmp 0x0:main

;-----------------------------------------
;	Main
;	Called with
;	CS = 0
;	DL = drive number				=> Stage1
;-----------------------------------------
main:
	;set segment registers to 0
	xor ax, ax
	mov ds, ax
	mov ss, ax
	;Now we like interrupts again
	sti

	;Look at first partition entry
	mov bx, PT1
	;There are four partition entries
	mov cx, 4

	;Enter lookup loop
	.loop:
		;Get byte containing bootable bit
		GetMemberByte(al,bx,PartitionEntry.DriveAttributes)
		;Test if partition is bootable
		test al, 0x80
		jnz .found
		;Select next entry
		add bx, 0x10
		;Decrement counter
		dec cx
		;if cx is bigger than 0 go to execute loop
		jnz .loop
	;if not go to error
	.error:
		cli
		hlt
	.found:
		;save partition entry offset
		push bx

		;test for bios lba extensions
		call test_bios_extension

		;get partition entry offset back
		pop bx
		push bx
		;get lba of the partition entry
		GetMemberDWord(eax,bx,PartitionEntry.LBA)
		;load one sector
		mov cx, 1
		;load to es:bx 0x0:0x7c00
		xor bx, bx
		mov es, bx
		mov bx, 0x7c00
		;call load_sector_lba
		call load_sector_lba

	;pop partition entry offset and save it to si
	pop si
	;pop boot drive number
	pop dx
	;jump to vba bootsector
	jmp 0x0:0x7c00

;-----------------------------------------
;	Include routines
;-----------------------------------------
%include "../Help_Functions/memory_lba.inc"

;-----------------------------------------
;	MBR data
;-----------------------------------------
times 0x1B8-($-$$) db 0

UID: times 6 db 0				; Unique Disk ID
PT1:
	istruc PartitionEntry
	iend						; First Partition Entry
PT2:
	istruc PartitionEntry
	iend						; Second Partition Entry
PT3:
	istruc PartitionEntry
	iend						; Third Partition Entry
PT4:
	istruc PartitionEntry
	iend						; Fourth Partition Entry

dw 0xAA55

;-----------------------------------------
;	Variables
;-----------------------------------------
%include "../Help_Functions/memory_lba_var.inc"