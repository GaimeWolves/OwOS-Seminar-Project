[bits 16]
[org 0x9000]
;-----------------------------------------
;	Main
;	Called with
;	CS = 0
;	DL = drive number				=> Kernel
;	DS:SI = partition table entry	=> Kernel
;-----------------------------------------
cli
hlt

;-----------------------------------------
;	Constants
;-----------------------------------------
STACK_POSITION					equ 0xFFFF
KERNEL_LOAD_SEGMENT				equ 0x1000
KERNEL_LOAD_OFFSET				equ 0x0000
KERNEL_ADDRESS					equ 0x100000

;-----------------------------------------
;	Variables
;-----------------------------------------
FileName:						db "KERNEL  SYS"

;-----------------------------------------
;	Includes
;-----------------------------------------
%include "../Help_Functions/struct.inc"
%include "../Help_Functions/memory_lba.inc"
%include "../Help_Functions/FAT32_FAT.inc"
%include "../Help_Functions/FAT32_RootDir.inc"
%include "../Help_Functions/MBR.inc"
%include "../Help_Functions/FAT32_struct.inc"

;-----------------------------------------
[bits 32]
;-----------------------------------------
;	Main32
;	Called with
;	CS = 0
;	DL = drive number				=> Kernel
;	DS:SI = partition table entry	=> Kernel
;-----------------------------------------
main32:
	cli
	hlt