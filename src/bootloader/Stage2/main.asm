[bits 16]
[org 0x9000]

;-----------------------------------------
;	Includes (Preprocessor)
;-----------------------------------------
%include "../Help_Functions/struct.inc"
%include "../Help_Functions/MBR.inc"
%include "../Help_Functions/FAT32_struct.inc"
%include "Multiboot.inc"

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
		;Get lower memory size
		int 0x12
		jc error
		mov WORD[LowerMem], ax
		;Get upper memory size
		call GetUpperMem
		mov DWORD[UpperMem], eax

		;Get memory map
		push MMAP_SEGMENT
		pop es
		mov di, MMAP_OFFSET
		call GetMMap
		;Save size of MMap
		sub di, MMAP_OFFSET
		mov WORD[MMapSize], di
		;Activate 32 bits
		%include "32bit.inc"
		;Should not return

	error:
		cli
		hlt

;-----------------------------------------
;	Constants
;-----------------------------------------
STACK_POSITION					equ 0xFFFF
STACK_POSITION_32				equ 0xFFFFFF
KERNEL_LOAD_SEGMENT				equ 0x1000
KERNEL_LOAD_OFFSET				equ 0x0000
MMAP_SEGMENT					equ 0x0000
MMAP_OFFSET						equ 0x1000
MULTIBOOT_ADDRESS				equ 0x0900

;-----------------------------------------
;	Variables
;-----------------------------------------
BootloaderName:					db "Molyload", 0x00
MMapSize:						dd 0
FileName:						db "KERNEL  ELF"
KernelSectorCount:				dw 0
UpperMem:						dd 0
LowerMem:						dd 0
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
%include "mmap.inc"
%include "upper_mem.inc"

;-----------------------------------------
[bits 32]
;-----------------------------------------
;	Includes (32 bits)
;-----------------------------------------
%include "./ELF/elf_header.inc"

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

	;Save address of partition entry and drive number
	push ax
	push dx

;-----------------------------------------
;	main32
;	Called with
;	AX = address of partition entry => Kernel
;	DL = drive number				=> Kernel
;-----------------------------------------
main32:
	;Setup multiboot struct
	mov DWORD[MULTIBOOT_ADDRESS + Multiboot.flags], 0b1001000011			;Activate fields with flags[0,1,6,9]

	;Upper Mem & Lower Mem
	mov eax, DWORD[LowerMem]
	mov DWORD[MULTIBOOT_ADDRESS + Multiboot.mem_lower], eax					;Save lower memory size to Multiboot struct
	mov eax, DWORD[UpperMem]
	mov DWORD[MULTIBOOT_ADDRESS + Multiboot.mem_upper], eax					;Save upper memory size to Multiboot struct

	;MMap
	mov eax, DWORD[MMapSize]
	mov DWORD[MULTIBOOT_ADDRESS + Multiboot.mmap_length], eax			 	;Save MMapSize to Multiboot struct
	
	;Calculate MMap address
	mov eax, MMAP_SEGMENT
	shl eax, 4
	add eax, MMAP_OFFSET
	mov DWORD[MULTIBOOT_ADDRESS + Multiboot.mmap_addr], eax					;Save MMap address to Multiboot struct
	
	;Bootloader name
	lea eax, [BootloaderName]
	mov DWORD[MULTIBOOT_ADDRESS + Multiboot.boot_loader_name], eax			;Save pointer to bootloader name to Multiboot struct
	
	;Bootdevice
	pop ax																			;Get drive number
	mov BYTE[MULTIBOOT_ADDRESS + Multiboot.boot_device + boot_device.drive], al		;Save drive number to Mutiboot struct
	;Get part1
	pop ax																			;Get address of partition entry
	mov cx, WORD[0x7c00 + BPB.BytesPerSector]										;Get bytes per sector
	div cx																			;dx contains offset in sector
	sub dx, 0x1be																	;Partition entries start at 0x1be sector offset
	mov cx, 0x10
	mov ax, dx
	div cx																			;An entry is 16 bits; ax => contains partition number
	mov BYTE[MULTIBOOT_ADDRESS + Multiboot.boot_device + boot_device.part1], al		;Set partition number in Multiboot struct
	;We only support 1 partition
	mov BYTE[MULTIBOOT_ADDRESS + Multiboot.boot_device + boot_device.part2], 0xFF	;Set unused to 0xFF
	mov BYTE[MULTIBOOT_ADDRESS + Multiboot.boot_device + boot_device.part3], 0xFF	;Set unused to 0xFF

	;Parse ELF
	mov eax, KERNEL_LOAD_SEGMENT				;Segment needs to be shifted up 4 times
	shl eax, 4
	add eax, KERNEL_LOAD_OFFSET					;Add offset

	call SetupELFSections
	mov edx, eax

	;Set Register for Multiboot
	mov eax, 0x2BADB002
	mov ebx, MULTIBOOT_ADDRESS

	;Jump to kernel
	jmp edx