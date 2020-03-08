[bits 16]
[org 0x7c00]

;Jump to the code
jmp main

;*********************************************
;	BIOS Parameter Block
;*********************************************

; BPB Begins 3 bytes from start. We do a far jump, which is 3 bytes in size.
; If you use a short jump, add a "nop" after it to offset the 3rd byte.
times 3-($-$$) db 0
bpbOEM:						db "My OS   "			; OEM identifier 				(8 bytes)
bpbBytesPerSector:  		DW 512					; Number of bytes per sector	(2 bytes)
bpbSectorsPerCluster: 		DB 1					; Number of sectors per cluster	(1 byte )
bpbReservedSectors: 		DW 1					; Number of reserved sectors	(2 bytes)
bpbNumberOfFATs: 			DB 2					; Number of FATs				(1 byte )
bpbRootEntries: 			DW 0					; Number of directory entries	(2 bytes) (Must be set so that the root directory occupies entire sectors)
bpbTotalSectors: 			DW 0					; Total count of sectors		(2 bytes) (If this value is 0, it means there are more than 65535 sectors in the volume, and the actual count is stored in the Large Sector Count entry at 0x20)
bpbMedia: 					DB 0xF8					; Media descriptor type			(1 byte )
bpbSectorsPerFAT: 			DW 0					; (FAT12/16 only)				(2 bytes)
bpbSectorsPerTrack: 		DW 0					; Number of sectors per track	(2 bytes)
bpbHeadsPerCylinder: 		DW 2					; Number of heads				(2 bytes)
bpbHiddenSectors: 			DD 0					; Number of hidden sectors		(4 bytes)
bpbTotalSectorsBig:			DD 0					; Total count of sectors (large)(4 bytes)
bsSizeOfFAT: 				DD 0					; Size of FAT in sectors		(4 bytes)
bsFlags: 					DW 0					; Flags							(2 bytes)
bsFATVersion:		 		DW 0					; FAT version					(2 byteS)
bsRootDirCluster:			DD 0x02					; First cluster of the root dir	(4 bytes)
bsFSInfoSector:				DW 0					; Sector of the FSInfo struct	(2 bytes)
bsBackupBootSector:			DW 0					; Sector of the backupbootsector(2 bytes)
							DQ 0					; Reserved
							DD 0					; Reserved						(12 bytes)
bsDriveNumber: 				DB 0					; Drive Number					(1 byte )
							DB 0					; Reserved						(1 byte )
bsExtBootSignature: 		DB 0x29					; Signature						(1 byte )
bsSerialNumber:				DD 0xa0a1a2a3			; Serial Number					(4 bytes)
bsVolumeLabel: 				DB "OwOS FLOPPY"		; Label String					(11 bytes)
bsFileSystem: 				DB "FAT32   "			; System Identifier String		(8 bytes)

;-----------------------------------------
;	Constants
;-----------------------------------------
STACK_POSITION					equ 0xFFFF

;-----------------------------------------
;	Variables (preset)
;-----------------------------------------
FileName:						db "STAGE2  SYS"

;-----------------------------------------
;	Includes
;-----------------------------------------
%include "../Help_Functions/memory_lba.inc"
%include "../Help_Functions/FAT32_FAT.inc"
%include "../Help_Functions/FAT32_RootDir.inc"
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

;Set stack related registers to zero
xor ax, ax
mov ss, ax

mov sp, STACK_POSITION

;Save the variables passed by the MBR bootstrap code
mov BYTE[Drive_Number], dl
push si ;Pass to Stage2
push ds ;Pass to Stage2

;Get Partition LBA offset
GetMemberDWord(ebx,si,PartitionEntry.LBA) ;LBA offset is saved in the partition entry struct in ds:si 
mov DWORD[Partition_Table_LBA_Offset], ebx

;set segment register to 0
mov ds, ax


;Now interrupt are safe
sti

lea di, [FileName]
call SearchFile

xor cx, cx
mov ds, cx
mov si, 0x9000
loadLoop:
	mov dl, BYTE[Drive_Number]
	mov ebx, DWORD[Partition_Table_LBA_Offset]
	push eax
	call LoadCluster
	pop eax
	mov ebx, DWORD[Partition_Table_LBA_Offset]
	mov dl, BYTE[Drive_Number]
	push si
	call GetNextCluster
	pop si
	cmp eax, 0xFFFFFF8
	jl loadLoop

mov dl, BYTE[Drive_Number]
pop ds ;Instantly pushed after entering Stage1
pop si ;Instantly pushed after entering Stage1

jmp 0x0:0x9000


;Fill sector and write magic WORD
times 510-($-$$) db 0x0
dw 0xAA55
;-----------------------------------------
;	Variables
;-----------------------------------------
Drive_Number: 					db 0
Partition_Table_LBA_Offset:		dd 0

%include "../Help_Functions/memory_lba_var.inc"
%include "../Help_Functions/FAT32_FAT_var.inc"
%include "../Help_Functions/FAT32_RootDir_var.inc"