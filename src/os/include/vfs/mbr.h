#ifndef _MBR_H
#define _MBR_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define MBR_MAX_PARTITIONS 4

// Drive types (only contains used entries)
#define FAT32_LBA 0x0C // FAT32 partition with LBA addressing (most common)
#define GPT_HMBR  0xED // GPT hybrid MBR
#define GPT_PMBR  0xEE // GPT protective MBR identifier
#define EFI_PART  0xEF // EFI Partition

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

typedef struct partition_t
{
	bool used;
	int device;      // Device identifier (equals ATA number as we only support ATA)
	bool active;     // Boot partition
	uint64_t offset; // Offset in drive (sectors)
	uint64_t size;   // Size of partition (sectors)
} partition_t;

typedef struct disk_t
{
	uint64_t size; // Size of device (sectors)
	partition_t partitions[MBR_MAX_PARTITIONS]; // Partition list
} disk_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

int initMBR();
disk_t getPartitionInfo(int device);

#endif // _MBR_H
