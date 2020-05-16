#include <vfs/mbr.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <string.h>

#include <hal/atapio.h>
#include <debug.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define ACTIVE 0x80 // Active partition
#define UNUSED 0x00 // Indicator for unused partition entry

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

typedef struct entry_t
{
	uint8_t active;

	uint8_t startHead;
	uint16_t startSector : 6;
	uint16_t startCylinder : 10;

	uint8_t systemID;

	uint8_t endHead;
	uint16_t endSector : 6;
	uint16_t endCylinder : 10;

	uint32_t lbaOffset;
	uint32_t size;
} __attribute__((packed)) entry_t;

typedef struct mbr_t
{
	uint8_t bootstrap[446];              // MBR bootstrap code
	entry_t entries[MBR_MAX_PARTITIONS]; // Partition entries
	uint16_t bootsignature;              // 0x55AA
} __attribute__((packed)) mbr_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

// Information about all devices
disk_t disks[ATA_MAX_DRIVES];

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static int parseMBR(int device);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Parses the MBR of a drive
static int parseMBR(int device)
{
	// Get drive information from ATA driver
	drive_t driveInfo = getDrive(device);
	
	if (!driveInfo.inserted)
		return -1;

	disks[device].size = driveInfo.size;

	// Read in MBR
	mbr_t mbr;
	if (ataRead((void*)&mbr, 0, 1, device))
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Failed to read device");
		debug_set_color(0x0F, 0x00);
		return -1;
	}

	// Test for GPT partition
	{
		int id = mbr.entries[0].systemID;
		if (id == GPT_PMBR || id == GPT_HMBR || id == EFI_PART)
		{
			debug_set_color(0x0C, 0x00);
			debug_print("GPT is unsupported");
			debug_set_color(0x0F, 0x00);
			return -1;
		}
	}

	// Parse partition entries
	for (int i = 0; i < MBR_MAX_PARTITIONS; i++)
	{
		partition_t *current = &disks[device].partitions[i];
		current->active = mbr.entries[i].active == ACTIVE;
		current->used = mbr.entries[i].active != UNUSED;
		current->device = device;
		current->offset = mbr.entries[i].lbaOffset;
		current->size = mbr.entries[i].size;
		current->type = mbr.entries[i].systemID;
	}

	return 0;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Initializes partition list
int initMBR()
{
	int returnCode = 0;

	// Initialize partition list
	memset((void*)disks, 0, sizeof(disk_t) * ATA_MAX_DRIVES);

	for (int i = 0; i < ATA_MAX_DRIVES; i++)
		returnCode += parseMBR(i);

	return returnCode;
}

// Get the partitions for a drive
disk_t* getPartitionInfo(int device)
{
	if (device < 0 || device >= ATA_MAX_DRIVES)
		return NULL;

	return &disks[device];
}
