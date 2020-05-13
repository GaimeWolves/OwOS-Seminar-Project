#ifndef _ATAPIO_H
#define _ATAPIO_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

// Defines the drive to load from
// Used as index into device array
#define ATA_DRIVE_0 0x00
#define ATA_DRIVE_1 0x01
#define ATA_DRIVE_2 0x02
#define ATA_DRIVE_3 0x03

#define ATA_MAX_DRIVES 4

// Drive type number
#define ATA_DRIVE_TYPE_FIXED     0x01
#define ATA_DRIVE_TYPE_REMOVABLE 0x02
#define ATA_DRIVE_TYPE_UNKNOWN   0x03

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

typedef struct drive_t
{
	bool inserted;
	bool hasLBA48;
	uint64_t size;
	uint8_t type;
} drive_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

int initATA();

int ataRead(void* buf, uint64_t lba, uint32_t sectors, uint8_t drive);
int ataWrite(void* buf, uint64_t lba, uint32_t sectors, uint8_t drive);

drive_t getDrive(uint8_t drive);

#endif // _ATAPIO_H
