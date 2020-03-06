struc DAP_struct
	.Size:			resb 1
	.Reserved:		resb 1
	.Count:			resw 1
	.Offset:		resw 1
	.Segment:		resw 1
	.LBA_Low:		resd 1
	.LBA_High:		resd 1
endstruc

DAP:
	istruc DAP_struct
	iend