struc BPB
	.JMP:						resb 3
	.OEMId:						resb 8
	.BytesPerSector:			resw 1
	.SectorsPerCluster:			resb 1
	.ReservedSectors:			resw 1
	.NumberOfFATS:				resb 1
	.NumberOfDirectoyEntries:	resw 1
	.TotalSectors:				resw 1
	.MediaDescriptorType:		resb 1
	.FAT12Only:					resw 1
	.SectorsPerTrack:			resw 1
	.NumberOfHeads:				resw 1
	.HiddenSectors:				resd 1
	.TotalSectorsLarge:			resd 1
	.SectorsPerFAT:				resd 1
	.Flags:						resw 1
	.FATVersion:				resw 1
	.ClusterOfRootDir:			resd 1
	.SectorOfFSInfo:			resw 1
	.SectorOfBackupBoot:		resw 1
								resb 12 ;Reserved
	.DriveNumber:				resb 1
								resb 1  ;Reserved
	.Signature:					resb 1
	.VolumeID:					resd 1
	.VolumeLabel:				resb 11
	.SystemIdentifier:			resb 8  ; "FAT32   "
endstruc

struc Directory
	.FileName:					resb 11
	.Attributes:				resb 1
								resb 1  ;Reserved
	.CreationTimeSeconds:		resb 1
	.CreationTime:				resw 1
	.CreationDate:				resw 1
	.LastAccessDate:			resw 1
	.FirstClusterHigh:			resw 1
	.LastModificationTime:		resw 1
	.LastModificationDate:		resw 1
	.FirstClusterLow:			resw 1
	.Size:						resd 1
endstruc