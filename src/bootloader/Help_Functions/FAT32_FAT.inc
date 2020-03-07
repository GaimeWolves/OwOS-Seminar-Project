FATTempSectorOffset			equ 0x7f00
%include "../Help_Functions/FAT32_struct.inc"
;-----------------------------------------
;	LoadFATEntry
;	Called with
;	EAX = CurrentCluster
;	EBX = Partition LBA offset
;	 DL = DriveNumber
;	EAX => Offset in FAT
;-----------------------------------------
LoadFATEntry:
	;Save drive number
	push dx
	;calculate the sector offset of the cluster in the FAT
	xor edx, edx ;Null edx
	mov ecx, 16 ;There are 16 entries per FAT
	div ecx ;EAX = sector offset in FAT, EDX = offset in FAT sector

	;Save Offset in FAT and restore drive number to cx
	pop cx
	push edx

	;Load FAT sector
	xor edx, edx
	mov dx, [0x7c00 + BPB.ReservedSectors] ;FAT starts after reserved sectors
	add eax, edx ;Add FAT start to LBA
	add eax, ebx ;Add partition offset to LBA
	mov dx, cx ;Move drive number to dl
	mov cx, 0x01 ;Load 1 Sector
	mov bx, 0
	mov es, bx ;Segment 0
	mov bx, FATTempSectorOffset ;Offset FATTempSectorOffset
	call load_sector_lba

	;Restore offset in FAT sector
	pop eax
	ret

;-----------------------------------------
;	GetNextCluster
;	Called with
;	EAX = Current Cluster
;	EBX = Partition LBA offset
;	 DL = DriveNumber
;	EAX => NextCluster
;-----------------------------------------
GetNextCluster:
	;Load FAT sector containing the current cluster
	call LoadFATEntry
	;Load the next cluster with the calculated offset
	mov eax, [FATTempSectorOffset + EAX * 4]
	and eax, 0x0FFFFFFF ;A cluster entry is only 28 bits long

	ret