FATRootDirBufferOffset			equ 0x8100
;-----------------------------------------
;	SearchFile
;	Called with
;	EBX = Partition LBA offset
;	 DL = DriveNumber;
;	ES:DI = Reference to file name
;   EAX => First Cluster
;-----------------------------------------
SearchFile:
	;Save important arguments
	mov DWORD[PartitionOffset], ebx
	mov BYTE[DriveNumber], dl
	mov WORD[StringSegment], es
	mov WORD[StringOffset], di
	;Get first root dir cluster
	mov eax, DWORD[0x7c00 + BPB.ClusterOfRootDir]
	
	.nextCluster:
		;Load to segment 0x0
		xor cx, cx
		mov ds, cx
		;load to offset 0x8100
		mov si, FATRootDirBufferOffset
		;Push cluster
		push eax
		;Load cluster
		call LoadCluster ;Returns count of loaded in cx

		mov ax, cx
		mov cx, 0x10
		mul cx ;There are 16 32 bit entries in a sector

		;String entry in directory struct
		;ds = 0x0 => already contained
		mov cx, FATRootDirBufferOffset
		mov si, cx

		.compareLoop:
			;Restore string buffer
			mov es, WORD[StringSegment]
			mov di, WORD[StringOffset]
			;Test 11 chars
			mov cx, 0x0B
			;Test string
			push si
			rep cmpsb
			pop si
			;Found?
			je .found
			;Not Found
			dec ax ;Decrease outer counter
			je .finished ;If ax is 0
			add si, 0x20 ;if not increase go to next entry
			jmp .compareLoop
		.finished:
			;file not found in this cluster
			pop eax ;get currentCluster back
			;Restore arguments
			mov ebx, DWORD[PartitionOffset]
			mov dl, BYTE[DriveNumber]
			call GetNextCluster ;get next cluster
			;Test if this was the last cluster
			cmp eax, 0xFFFFFF8
			jge .end
			;GetNextCluster overwrites ebx
			mov ebx, DWORD[PartitionOffset]
			jmp .nextCluster
		.found:
			;Stack integrity
			pop eax
			;file entry in ds:si
			;get first cluster
			GetMemberWord(ax,si,Directory.FirstClusterHigh) ;high 16 bits
			shl eax, 0x10 ;move to the high 16 bits in eax
			GetMemberWord(ax,si,Directory.FirstClusterLow) ;low 16 bits
			ret
	.end:
	cli
	hlt