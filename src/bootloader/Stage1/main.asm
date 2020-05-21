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
STAGE2_OFFSET					equ 0x9000

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

;Set stack related registers
push 0
pop ss					;Stack segment = 0
mov sp, STACK_POSITION	;Stack pointer = STACK_POSITION

;Save the variables passed by the MBR bootstrap code
push si ;Pass to Stage2
push ds ;Pass to Stage2
push dx ;Restore when loading the cluster chain and pass to Stage2

;Get partition's LBA offset
GetMemberDWord(ebx,si,PartitionEntry.LBA) ;LBA offset is saved in the partition entry struct in ds:si 
push ebx ;Restore when loading the cluster chain

;set segment registers to 0
push 0
pop ds
push 0
pop es
;code segment register already contains 0

;Now interrupt are safe
sti

;Search Stage2 file in the root dir
mov di, FileName ;"STAGE2  SYS"
call SearchFile

;load to 0x0:STAGE2_OFFSET
mov di, STAGE2_OFFSET	;Offset STAGE2_OFFSET
;Load the sectors of the clusterchain
pop ebx	;Restore partition's LBA offset
pop dx	;Restore drive number and pass it to Stage2

;Load cluster chain
call LoadClusterChain

;Get back Stage1 arguments and pass them to Stage2
;dl already set
;ds:si = partition entry
pop ds ;Pushed after entering Stage1
pop si ;Pushed after entering Stage1

;Jump to Stage2
jmp 0x0:STAGE2_OFFSET


;Fill sector and write magic WORD
times 510-($-$$) db 0x0
dw 0xAA55
;-----------------------------------------
;	Variables
;-----------------------------------------
%include "../Help_Functions/memory_lba_var.inc"
%include "../Help_Functions/FAT32_FAT_var.inc"
%include "../Help_Functions/FAT32_RootDir_var.inc"