FATTempSectorOffset			equ 0x7f00
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

	;Save currently loaded FAT sector
	mov DWORD[CurrentlyLoadedFAT], eax

	;If no FAT is loaded skip the test
	xor cx, cx
	mov cl, BYTE[LoadedFATInitialized]
	cmp cx, 0x00
	je .Continue

	;Test if the requested sector is loaded
	mov ecx, DWORD[CurrentlyLoadedFAT]
	cmp ecx, eax
	je .AlreadyLoaded

	.Continue:
	mov BYTE[LoadedFATInitialized], 0x01
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

	.AlreadyLoaded:
	pop cx
	mov eax, edx
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

;-----------------------------------------
;	LoadCluster
;	Called with
;	EAX = Current Cluster
;	EBX = Partition LBA offset
;	 DL = DriveNumber
;   DS:SI = pointer to memory buffer
;	CX => Loaded sector count
;-----------------------------------------
LoadCluster:
	;Save arguments
	push ds
	push si
	push ds
	push si
	push dx
	push eax
	;clear ds register
	xor cx, cx
	mov ds, cx
	;Calculate first data sector
	mov eax, DWORD[0x7c00 + BPB.SectorsPerFAT]
	xor ecx, ecx
	mov cl, BYTE[0x7c00 + BPB.NumberOfFATS]
	mul ecx ;eax = sectors used by FATs
	xor ecx, ecx
	mov cx, WORD[0x7c00 + BPB.ReservedSectors]
	add eax, ecx ;eax = first data sector

	;save to edx
	mov ecx, eax
	;get current cluster back
	pop eax
	;the first real cluster is two
	sub eax, 0x02

	;calculate sector
	xor edx, edx
	mov dl, BYTE[0x7c00 + BPB.SectorsPerCluster]
	mul edx ;eax = sector offset in data sectors
	add eax, ecx ;eax = sector offset in partition
	add eax, ebx ;eax = sector offset in disk

	;load sectors
	pop dx	;get drive number back
	xor cx, cx
	mov cl, BYTE[0x7c00 + BPB.SectorsPerCluster] ;load SectorsPerCluster sectors
	pop bx ;load to es:bx
	pop es ;load to es:bx

	call load_sector_lba

	;increase si
	mov ax, cx ;get count of loaded sectors
	mov bx, WORD[0x7c00 + BPB.BytesPerSector] ;get byte count of a sector
	mul bx ;calculate loaded bytes

	;restore memory buffer
	pop si
	pop ds

	add ax, si ;add si
	mov si, ax ;save result to si

	ret