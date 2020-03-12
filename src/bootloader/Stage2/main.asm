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
	;Reset data segment and save loaded sectors count
	push 0x00
	pop ds		;Set segment to 0
	mov WORD[KernelSectorCount], cx

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
		;Activate 32 bits
		%include "32bit.inc"
		;Should not return

	cli
	hlt

;-----------------------------------------
;	Constants
;-----------------------------------------
STACK_POSITION					equ 0xFFFF
STACK_POSITION_32				equ 0xFFFFFFFF
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
%include "32bit_var.inc"

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
;	setup32
;	Called with
;	AX = address of partition entry => Kernel
;	DL = drive number				=> Kernel
;-----------------------------------------
setup32:
	;Setup segment registers
	mov bx, 0x10	;Use 2nd descriptor
	mov ds, bx
	mov es, bx
	mov ss, bx
	mov fs, bx
	mov gs, bx

	;Set stack
	mov esp, STACK_POSITION_32

	;Save address of partition entry
	push ax
	push dx

;-----------------------------------------
;	main32
;	Called with
;	AX = address of partition entry => Kernel
;	DL = drive number				=> Kernel
;-----------------------------------------
main32:
	;Copy the kernel to its position
	xor eax, eax
	xor ecx, ecx
	mov ax, WORD[KernelSectorCount]				;Get kernel sector count
	mov cx, WORD[0x7c00 + BPB.BytesPerSector]	;Get bytes per sector
	mul ecx										;Calculate byte size of kernel
	mov ecx, eax								;Save it to ecx for use in rep

	;Calculate load address of the kernel
	mov esi, KERNEL_LOAD_SEGMENT				;Segment needs to be shifted up 4 times
	shl esi, 4
	add esi, KERNEL_LOAD_OFFSET					;Add offset

	mov edi, KERNEL_ADDRESS						;Load destination address of the kernel

	rep movsb									;Copy kernel's bytes
	
	;Setup multiboot struct

	cli
	hlt