[bits 16]
[org 0x9000]

;-----------------------------------------
;	Includes (Preprocessor)
;-----------------------------------------
%include "../Help_Functions/struct.inc"
%include "../Help_Functions/MBR.inc"
%include "../Help_Functions/FAT32_struct.inc"

;-----------------------------------------
;	Main
;	Called with
;	CS = 0
;	DL = drive number				=> Kernel
;	DS:SI = partition table entry	=> Kernel
;-----------------------------------------
	main:
	;We don't want to be interrupted currently
	cli

	;Set stack related registers
	push 0
	pop ss					;Stack segment = 0
	mov sp, STACK_POSITION	;Stack pointer = STACK_POSITION

	;Save the variables passed by the Stage1 bootstrap code
	push si ;Pass to Kernel
	push ds ;Pass to Kernel
	push dx ;Pass to Kernel
	push dx ;Restore when loading the cluster chain

	;Get partition's LBA offset
	GetMemberDWord(ebx,si,PartitionEntry.LBA) ;LBA offset is saved in the partition entry struct in ds:si 
	push ebx ;Restore when loading the cluster chain

	;set data segment register to 0
	push 0
	pop ds
	;code segment register already contains 0

	;Now interrupt are safe
	sti

	;Search kernel file in the root dir
	mov di, FileName ;"KERNEL  SYS"
	call SearchFile

	;load to 0x0:KERNEL_LOAD_OFFSET
	push KERNEL_LOAD_SEGMENT
	pop ds					;Segment KERNEL_LOAD_SEGMENT
	mov si, KERNEL_LOAD_OFFSET	;Offset KERNEL_LOAD_OFFSET
	;Load the sectors of the clusterchain
	pop ebx	;Restore partition's LBA offset
	pop dx	;Restore drive number

	;Load cluster chain
	call LoadClusterChain

	;Enable A20 line
	call a20EnabledTest
	cmp ax, 0x01
	je .a20enabled
	call a20KeyboardControllerCommand
	cmp ax, 0x01
	je .a20enabled
	;If activation failed: stop
	cli
	hlt
	
	.a20enabled:

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
KernelSectorCount:				dw 0
%include "../Help_Functions/memory_lba_var.inc"
%include "../Help_Functions/FAT32_FAT_var.inc"
%include "../Help_Functions/FAT32_RootDir_var.inc"

;-----------------------------------------
;	Includes
;-----------------------------------------
%include "../Help_Functions/memory_lba.inc"
%include "../Help_Functions/FAT32_FAT.inc"
%include "../Help_Functions/FAT32_RootDir.inc"
%include "a20.inc"

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