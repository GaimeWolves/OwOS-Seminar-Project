#include <hal/atapio.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

#include <hal/cpu.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define DRIVE_TYPE_FIXED     0x01
#define DRIVE_TYPE_REMOVABLE 0x02

// For readability
#define PRIMARY_BUS   0x00
#define SECONDARY_BUS 0x01
#define MASTER_DRIVE  0x00
#define SLAVE_DRIVE   0x01

// Base addresses of the I/O Registers
#define PRIMARY_IO_BASE     0x1F0
#define PRIMARY_CTRL_BASE   0x3F6
#define SECONDARY_IO_BASE   0x170
#define SECONDARY_CTRL_BASE 0x376

// Offsets for specific registers
#define REG_ERROR    0x01
#define REG_FEATURE  0x01
#define REG_COUNT    0x02
#define REG_LBA_LOW  0x03
#define REG_LBA_MID  0x04
#define REG_LBA_HIGH 0x05
#define REG_DRIVE    0x06
#define REG_STATUS   0x07
#define REG_COMMAND  0x07

#define REG_DRIVE_ADDR 0x01

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

// Is overloaded with useless information
// only usefull info is included
typedef struct identify_data_t
{
	// Encompasses the first uint16_t
	// Holds general information about drive
	struct {
		uint16_t reserved1 : 6;
		uint16_t fixedMedia : 1;     // Hard drives
		uint16_t removableMedia : 1; // USB Sticks, etc.
		uint16_t reserved2 : 7;
		uint16_t deviceType : 1;     // Is ATA device
	} general;

	uint16_t unused1[59];

	uint32_t sectorCount; // Total sector count (LBA28)

	uint16_t unused2[21];
	uint16_t unused3 : 9;

	uint16_t hasLBA48 : 1; // Is LBA48 supported
	
	uint16_t unused4 : 6;
	uint16_t unused5[15];

	uint64_t extSectorCount; // LBA48-addressable sector count

	uint16_t unused6[152];
} __attribute__((packed)) identify_data_t;


//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

// Holds information about drives
static drive_t drives[4];

// Save current and last used drive to reduce unnecessary I/O instructions
static uint8_t current = 4, last = 4;

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static uint8_t index(uint8_t bus, uint8_t drive);
static uint8_t bus(uint8_t drive);
static uint8_t drive(uint8_t drive);

static void toggleInterrupts(bool on);
static void selectDrive(uint8_t bus, uint8_t drive);
static void delay(uint8_t bus);
static identify_data_t identify(uint8_t bus, uint8_t drive);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Select the specified drive
static void selectDrive(uint8_t bus, uint8_t drive)
{
	uint8_t driveIndex = index(bus, drive);

	if (driveIndex == current)
		return;
	
	last = current;
	current = driveIndex;

	uint16_t port = REG_DRIVE + (bus == PRIMARY_BUS ? PRIMARY_IO_BASE : SECONDARY_IO_BASE);
	uint16_t value = 0xE0 | (drive << 4); // Sets slave bit if drive is 1 (SLAVE_DRIVE)

	outb(port, value);
}

// Creates a 400ns delay by polling the alt status register four times
static void delay(uint8_t bus)
{
	uint16_t port = bus == PRIMARY_BUS ? PRIMARY_CTRL_BASE : SECONDARY_CTRL_BASE;
	for (int i = 0; i < 4; i++)
		inb(port);
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Initializes the driver and gets drive information
int initATA()
{
	return 0;
}

// Reads a single sector from the drive
int ataReadOne(void* buf, uint64_t lba, uint8_t drive)
{
	return 0;
}

// Reads the specified amount of sectors from the drive
int ataRead(void* buf, uint64_t lba, uint64_t sectors, uint8_t drive)
{
	return 0;
}

// Gets information about a drive
drive_t getDrive(uint8_t drive)
{
	if (drive >= 4) // Return "zero" struct on error
		return (const drive_t){ false, false, 0, 0 };

	return drives[drive];
}
