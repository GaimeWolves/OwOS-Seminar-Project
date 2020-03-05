struc PartitionEntry
    .DriveAttributes:   resb 1
    .CHS_Start:         resb 3
    .Type:              resb 1
	.CHS_End:			resb 3
	.LBA:				resd 1
	.Size:				resd 1
endstruc