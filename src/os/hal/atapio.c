#include <hal/atapio.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <hal/cpu.h>
#include <hal/pic.h>
#include <hal/interrupt.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

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
// I/O base registers
#define REG_DATA     0x00 // Read/Write to receive/send data to drive
#define REG_ERROR    0x01 // Holds error information
#define REG_FEATURE  0x01 // Command specific interface
#define REG_COUNT    0x02 // Sector count register
#define REG_LBA_LOW  0x03 // Low disk sector address
#define REG_LBA_MID  0x04 // Mid disk sector address
#define REG_LBA_HIGH 0x05 // High disk sector address
#define REG_DRIVE    0x06 // Used to set active drive and other flags
#define REG_STATUS   0x07 // Status register
#define REG_COMMAND  0x07 // Command register

// Control base registers
#define REG_ALT_STATUS 0x00 // Alternate status (does not affect IRQs)
#define REG_DEVICE_CTL 0x00 // Used to reset the bus or toggle interrupts
#define REG_DRIVE_ADDR 0x01 // Provides information about drive state

// IRQ numbers (unused but here for completness)
#define PRIMARY_IRQ   32 + 14
#define SECONDARY_IRQ 32 + 15

// Command numbers
#define CMD_IDENTIFY          0xEC // Get information about a drive
#define CMD_CLEAR_CACHE       0xE7 // Clear the write cache
#define CMD_READ_SECTORS      0x20 // Read sectors (LBA28)
#define CMD_READ_SECTORS_EXT  0x24 // Read sectors (LBA48)
#define CMD_WRITE_SECTORS     0x30 // Write sectors (LBA28)
#define CMD_WRITE_SECTORS_EXT 0x34 // Write sectors (LBA48)

// Important constants
#define NUM_WORDS     256            // Number of words inside one sector
#define LBA28_SECTORS 256            // Highest number of sectors readable (128 KiB)
#define LBA48_SECTORS 65536          // Highest number of sectors readable (32 Mib)
#define LBA28_MAX 0x0FFFFFFF         // Highest addressable sector (128GB)
#define LBA48_MAX 0x0000FFFFFFFFFFFF // Highest addressable sector (more than enough petabytes)

// Modes for combining read and write functionality into one method
#define READ_MODE false
#define WRITE_MODE true

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
} __attribute__((aligned(4),packed)) identify_data_t;

// Holds the status bits as one byte
// The union makes it possible to cast the status value to a status struct
typedef union status_t
{
	struct {
		uint8_t ERR : 1;  // Error bit
		uint8_t IDX : 1;  // Index bit (internal use)
		uint8_t CORR : 1; // Corrected data (internal use)
		uint8_t DRQ : 1;  // PIO Data sendable/receivable
		uint8_t SRV : 1;  // Service request
		uint8_t DF : 1;   // Drive fault error (doesnt set ERR)
		uint8_t RDY : 1;  // Ready byte
		uint8_t BSY : 1;  // Drive is busy
	};
	uint8_t byte;
} __attribute__((packed)) status_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

// Holds information about drives
static drive_t drives[ATA_MAX_DRIVES];

// Save current and last used drive to reduce unnecessary I/O instructions
static uint8_t current = ATA_MAX_DRIVES;
static uint8_t last = ATA_MAX_DRIVES;

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static inline uint8_t index(uint8_t bus, uint8_t drive);
static inline uint8_t bus(uint8_t drive);
static inline uint8_t drv(uint8_t drive);

static inline uint16_t ioPort(uint8_t bus);
static inline uint16_t ctrlPort(uint8_t bus);

static void disableInterrupts(bool disable);
static void selectDrive(uint8_t bus, uint8_t drive);

static void delay(uint8_t bus);
static int poll(uint8_t bus);

static int identify(uint8_t bus, uint8_t drive, identify_data_t *data);
static void detectDrives();

static int doPIOTransfer(uint16_t *buf, uint8_t bus, uint8_t drive, uint64_t lba, uint32_t sectors, bool useLBA48, bool mode);
static int doDataTansfer(void* buf, uint64_t lba, uint32_t sectors, uint8_t drive, bool mode);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Implemented IRQs for completness as disabling interrupts may fail
// The interrupts dont need to do anything because the driver is polling
__interrupt_handler static void primaryIRQ(InterruptFrame_t* frame)
{
	endOfInterrupt(PRIMARY_IRQ);
}

__interrupt_handler static void secondaryIRQ(InterruptFrame_t* frame)
{
	endOfInterrupt(SECONDARY_IRQ);
}

// Gets the drive index from the bus and drive values
static inline uint8_t index(uint8_t bus, uint8_t drive)
{
	return (bus << 1) | drive;
}

// Gets the respective bus for a drive index
static inline uint8_t bus(uint8_t drive)
{
	return drive >> 1;
}

// Gets the respective drive for a drive index
static inline uint8_t drv(uint8_t drive)
{
	return drive & 0x01;
}

// Returns the IO port base of the bus
static inline uint16_t ioPort(uint8_t bus)
{
	return bus == PRIMARY_BUS ? PRIMARY_IO_BASE : SECONDARY_IO_BASE;
}

// Returns the control port base of the bus
static inline uint16_t ctrlPort(uint8_t bus)
{
	return bus == PRIMARY_BUS ? PRIMARY_CTRL_BASE : SECONDARY_CTRL_BASE;
}

// Disables interrupts by setting nIEN on every drive
static void disableInterrupts(bool disable)
{
	for (int i = 0; i < ATA_MAX_DRIVES; i++)
	{
		selectDrive(bus(i), drv(i));
		outb(ctrlPort(bus(i)) + REG_DEVICE_CTL, disable << 1);
	}
}

// Select the specified drive
static void selectDrive(uint8_t bus, uint8_t drive)
{
	uint8_t driveIndex = index(bus, drive);

	if (driveIndex == current)
		return;
	
	last = current;
	current = driveIndex;

	uint16_t value = 0xE0 | (drive << 4); // Sets slave bit if drive is 1 (SLAVE_DRIVE)
	outb(ioPort(bus) + REG_DRIVE, value);
}

// Creates a 400ns delay by polling the alt status register four times
static void delay(uint8_t bus)
{
	uint16_t port = ctrlPort(bus);
	for (int i = 0; i < 4; i++)
		inb(port + REG_ALT_STATUS);
}

// Polls the status register until BSY clears and DRQ sets or ERR sets
// Returs error if ERR bit was set
static int poll(uint8_t bus)
{
	uint16_t port = ctrlPort(bus) + REG_ALT_STATUS;

	status_t status = (status_t)inb(port);

	while(status.BSY) // Wait for BSY to clear
		status.byte = inb(port );

	while(!status.DRQ) // Wait for DRQ to set
	{
		if (status.ERR) // Break on ERR
			return -1;

		status.byte = inb(port);
	}

	return 0;
}

// Tries the IDENTIFY command on a drive to get information about it
// Returns error if the drive is not connected
static int identify(uint8_t bus, uint8_t drive, identify_data_t *data)
{
	// Select needed drive
	selectDrive(bus, drive);
	uint16_t port = ioPort(bus);

	// Set needed registers to zero
	outb(port + REG_COUNT, 0);
	outb(port + REG_LBA_LOW, 0);
	outb(port + REG_LBA_MID, 0);
	outb(port + REG_LBA_HIGH, 0);

	// Send IDENTIFY command
	outb(port + REG_COMMAND, CMD_IDENTIFY);

	status_t status = (status_t)inb(port + REG_STATUS);

	// If status returns zero no drive is connected
	if (status.byte == 0)
		return -1;

	// Poll until the BSY flag clears
	while(status.BSY)
		status.byte = inb(port + REG_STATUS);

	// Check for non ATA drive
	if (inb(port + REG_LBA_MID) == 0 || inb(port + REG_LBA_HIGH) == 0) // ATA drive
	{
		// Poll until DRQ sets or ERR sets
		while (!status.DRQ)
		{
			status.byte = inb(port + REG_STATUS);

			if (status.ERR)
				return -1;
		}
	}

	if (status.ERR)
		return -1;

	// Read data into struct
	uint16_t *buf = (uint16_t*)data;
	for (int i = 0; i < NUM_WORDS; i++)
		buf[i] = inw(port + REG_DATA);

	return 0;
}

// Runs IDENTIFY on every drive and populates the drive array
static void detectDrives()
{
	identify_data_t data;
	for (int i = 0; i < ATA_MAX_DRIVES; i++)
	{
		// Initialize to "no drive inserted"
		drives[i] = (const drive_t){ false, false, 0, 0 };
		if (!identify(bus(i), drv(i), &data)) // Identify drive
		{
			drives[i].inserted = true;
			drives[i].hasLBA48 = data.hasLBA48;
			drives[i].size = data.hasLBA48 ? data.extSectorCount : data.sectorCount;
			
			// Determine drive type
			if (data.general.fixedMedia)
				drives[i].type = ATA_DRIVE_TYPE_FIXED;
			else if (data.general.removableMedia)
				drives[i].type = ATA_DRIVE_TYPE_REMOVABLE;
			else
				drives[i].type = ATA_DRIVE_TYPE_UNKNOWN;
		}
	}
}


// Does a PIO transfer by reading from or into the specified drive
// Handles both LBA modes and both reads and writes as the logic only changes minimally
static int doPIOTransfer(uint16_t *buf, uint8_t bus, uint8_t drive, uint64_t lba, uint32_t sectors, bool useLBA48, bool mode)
{
	uint16_t port = ioPort(bus);

	// Different initialization procedure
	if (useLBA48) // LBA48 Mode
	{
		outb(port + REG_DRIVE, 0x40 | (drive << 4));     // Set correct mode
		outb(port + REG_COUNT, (uint8_t)(sectors >> 8)); // Count high byte
		outb(port + REG_LBA_LOW, (uint8_t)(lba >> 24));  // LBA4
		outb(port + REG_LBA_MID, (uint8_t)(lba >> 32));  // LBA5
		outb(port + REG_LBA_HIGH, (uint8_t)(lba >> 40)); // LBA6
		outb(port + REG_COUNT, (uint8_t)sectors);        // Count low byte
		outb(port + REG_LBA_LOW, (uint8_t)lba);          // LBA1
		outb(port + REG_LBA_MID, (uint8_t)(lba >> 8));   // LBA2
		outb(port + REG_LBA_HIGH, (uint8_t)(lba >> 16)); // LBA6

		// Send command	
		uint8_t data = mode == READ_MODE ? CMD_READ_SECTORS_EXT : CMD_WRITE_SECTORS_EXT;
		outb(port + REG_COMMAND, data);
	}
	else // LBA28 mode
	{
		// Also contains highest four bits of lba address
		outb(port + REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F)); // Set correct mode
		outb(port + REG_COUNT, (uint8_t)sectors);        // Sector count
		outb(port + REG_LBA_LOW, (uint8_t)lba);          // LBA low byte
		outb(port + REG_LBA_MID, (uint8_t)(lba >> 8));   // LBA middle byte
		outb(port + REG_LBA_HIGH, (uint8_t)(lba >> 16)); // LBA high byte
		
		// Send command
		uint8_t data = mode == READ_MODE ? CMD_READ_SECTORS : CMD_WRITE_SECTORS;
		outb(port + REG_COMMAND, data);
	}

	// 0 equals max number
	if (sectors == 0)
		sectors = useLBA48 ? LBA48_SECTORS : LBA28_SECTORS;

	// Same way of receving data
	while(sectors--)
	{
		// Poll until data is ready
		if (poll(bus))
			return -1;

		// Read data into buffer
		for (int i = 0; i < NUM_WORDS; i++)
		{
			if (mode == READ_MODE)
				*buf++ = inw(port + REG_DATA); // Read from drive
			else
				outw(port + REG_DATA, *buf++); // Write into drive
		}

		// Create 400ns delay to let the drive setup next PIO transfer
		delay(bus);
	}

	// Flush cache after a WRITE command
	// Not doing so can lead to successive WRITE commands failing
	if (mode == WRITE_MODE)
	{
		uint16_t ctrl = ctrlPort(bus);

		// Send CLEAR CACHE command to drive
		outb(port + REG_COMMAND, CMD_CLEAR_CACHE);
		
		// Wait for BSY to clear
		status_t status = (status_t)inb(ctrl + REG_ALT_STATUS);
		while(status.BSY)
			status.byte = inb(ctrl + REG_ALT_STATUS);
	}

	return 0;
}

// Does a complete data transfer by doing multiple PIO transfers with at max 256/65536 sectors
static int doDataTansfer(void* buf, uint64_t lba, uint32_t sectors, uint8_t drive, bool mode)
{
	// Error handling
	if (!buf)
		return -1;

	if (sectors == 0)
		return 0;

	if (drive >= ATA_MAX_DRIVES)
		return -1;

	if (!drives[drive].inserted)
		return -1;

	if (lba + sectors > (drives[drive].hasLBA48 ? LBA48_MAX : LBA28_MAX))
		return -1;

	if (drives[drive].size < lba + sectors)
		return -1;

	// If possible use LBA28 because its faster
	bool useLBA48 = lba + sectors > LBA28_MAX;
	uint32_t divisor = useLBA48 ? LBA48_SECTORS : LBA28_SECTORS;

	// Round up to full PIO transfers and divide by transfer size
	uint32_t count = ((sectors + divisor - 1) & -divisor) / divisor;
	
	// Do the needed number of PIO transfers
	for (uint32_t i = 0; i < count; i++)
	{
		// 0 means max amount
		uint32_t amount = sectors - i * divisor >= divisor ? 0 : sectors % divisor;
		uint16_t *bufOffset = (uint16_t*)((uintptr_t)buf + i * divisor * NUM_WORDS);
		uint64_t lbaOffset = lba + i * divisor;

		doPIOTransfer(bufOffset, bus(drive), drv(drive), lbaOffset, amount, useLBA48, mode);
	}

	return 0;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Initializes the driver and gets drive information
int initATA()
{
	// Set interrupts for completness sake
	setVect(PRIMARY_IRQ, (interruptHandler_t)primaryIRQ);
	setVect(SECONDARY_IRQ, (interruptHandler_t)secondaryIRQ);

	// Disable interrupts because polling is better with singletasking
	disableInterrupts(true);

	// Get drive information
	detectDrives();

	return 0;
}

// Reads the specified amount of sectors from the drive
int ataRead(void* buf, uint64_t lba, uint32_t sectors, uint8_t drive)
{
	return doDataTansfer(buf, lba, sectors, drive, READ_MODE);
}

// Writes the specified amount of sectors to the drive
int ataWrite(void* buf, uint64_t lba, uint32_t sectors, uint8_t drive)
{
	return doDataTansfer(buf, lba, sectors, drive, WRITE_MODE);
}

// Gets information about a drive
drive_t getDrive(uint8_t drive)
{
	if (drive >= ATA_MAX_DRIVES) // Return "zero" struct on error
		return (const drive_t){ false, false, 0, 0 };

	return drives[drive];
}
