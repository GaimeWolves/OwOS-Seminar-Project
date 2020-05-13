#include <memory/pmm.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <string.h>

#include <debug.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

enum MemoryType
{
	Free = 1,     // Available memory
	Reserved = 2, // Reserved memory
	ACPI = 3,     // ACPI Memory
	Preserve = 4, // Reserved memory (preserve on hibernation)
	Defective = 5 // Defective memory
};

// Strings for printing the memory map
char* TypeStrings[] = {
	"Available",
	"Reserved",
	"ACPI Memory",
	"Preserve",
	"Defective"
};

extern uintptr_t _end, _start; // Include linker variables for kernel size

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

static uint32_t memorySize = 0; // Size of memory
static uint32_t usedBlocks = 0; // Used block count
static uint32_t maxBlocks = 0;  // Max block count
static uint32_t *memoryMap = 0; // Holds the block bitmap

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static inline uint32_t freeBlockCount();

static inline void setBit(int32_t bit);
static inline void clearBit(int32_t bit);
static inline bool testBit(int32_t bit);

static int32_t firstFreeBlock();
static int32_t firstFreeContinuous(size_t size);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Returns the number of free blocks
static inline uint32_t freeBlockCount()
{
	return maxBlocks - usedBlocks;
}

// Sets a bit inside the bitmap
static inline void setBit(int32_t bit)
{
	memoryMap[bit / 32] |= (1 << (bit % 32));
}

// Clears a bit inside the bitmap
static inline void clearBit(int32_t bit)
{
	memoryMap[bit / 32] &= ~(1 << (bit % 32));
}

// Checks if the given bit inside the bitmap is set
static inline bool testBit(int32_t bit)
{
	return memoryMap[bit / 32] & (1 << (bit % 32));
}

static int32_t firstFreeBlock()
{
	for (uint32_t i = 0; i < maxBlocks / 32; i++)
	{
		// Is the chunk full?
		if (memoryMap[i] == 0xFFFFFFFF)
			continue;

		// Iterate through the bits in the chunk
		for (int j = 0; j < 32; j++)
		{
			// Is the bit free?
			if (!(memoryMap[i] & (1 << j)))
				return i * 32 + j;
		}
	}

	return -1;
}

static int32_t firstFreeContinuous(size_t size)
{
	if (size == 0)
		return -1;

	if (size == 1)
		return firstFreeBlock();

	for (uint32_t i = 0; i < maxBlocks / 32; i++)
	{
		// Is the chunk full?
		if (memoryMap[i] == 0xFFFFFFFF)
			continue;

		// Iterate through the bits in the chunk
		for (int j = 0; j < 32; j++)
		{
			int bit = (1 << j);

			if (memoryMap[i] & bit)
				continue;

			int start = i * 32 + bit;

			// Check if the needed amount of blocks is free
			bool free = true;
			for (uint32_t count = 0; count < size; count++)
			{
				// Break if block is used
				if (testBit(start + count))
				{
					free = false;
					break;
				}
			}

			if (free)
				return i * 32 + j;
		}
	}

	return -1;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Initializes the PMM by creating the bitmap and filling it
// with the information from the multiboot header
int initPMM(multiboot_info_t *header)
{
	debug_set_color(0xF, 0x0);
	debug_print("Initializing Physical Memory Manager...");

	if (!(header->flags & 1)) // No memory info available
	{
		debug_set_color(0xC, 0x0);
		debug_print("Memory size not available! Aborting...");
		return -1;
	}

	// memory_hi is the memory size after 1MiB
	// so we add 1KiB to the value
	memorySize = header->memory_lo + 1024 + header->memory_hi;
	debug_printf("Available memory: %uKiB", memorySize);

	// Memory map goes directly after kernel
	memoryMap = (uint32_t*)&_end;
	maxBlocks = (memorySize * 1024) / PMM_BLOCK_SIZE;

	// Initially all memory is used
	usedBlocks = maxBlocks;
	memset(memoryMap, 0xFF, maxBlocks / 8); // 8 blocks per byte

	if (!(header->flags & 0x40)) // No memory map available
	{
		debug_set_color(0xC, 0x0);
		debug_print("Memory map not available! Aborting...");
		debug_set_color(0xF, 0x0);
		return -1;
	}

	debug_print("Parsing memory map:");

	// Iterate through the memory map
	multiboot_mmap_entry_t *region = (multiboot_mmap_entry_t*)(uintptr_t)header->mmap_addr;
	while((uintptr_t)region < header->mmap_addr + header->mmap_len)
	{
		// All other values are defined to be reserved memory
		if (region->type > Defective)
			region->type = Reserved;


		if (region->type == Free)
			debug_set_color(0xF, 0x0);
		else
			debug_set_color(0xE - region->type, 0x0);
		debug_printf("0x%.16llx-0x%.16llx - %s", region->addr, region->addr + region->len, TypeStrings[region->type - 1]);

		if (region->type == Free)
			pmmFreeRegion(region->addr, region->len);

		region = (multiboot_mmap_entry_t*)((uintptr_t)(region) + region->size + sizeof(region->size));
	}

	// Declare kernel and bitmap as used
	pmmAllocRegion((uintptr_t)&_start, (size_t)((&_end - &_start) + maxBlocks / 8));
	
	// Allocate first block holding original IVT and BIOS data
	pmmAllocRegion(0, PMM_BLOCK_SIZE); 

	debug_set_color(0xF, 0x0);
	debug_printf("Used blocks: %u   Free Blocks: %u", usedBlocks, freeBlockCount());

	return 0;
}


// Allocates blocks for a specific memory region
// (for example DMA Buffer, kernel, etc.)
// Returns the actual number of blocks allocated
int pmmAllocRegion(uintptr_t base, size_t size)
{
	int block = base / PMM_BLOCK_SIZE;
	int count = size / PMM_BLOCK_SIZE;

	// Check neccesary bits
	bool free = true;
	for (int i = 0; i <= count; i++)
	{
		if (testBit(block + i))
		{
			free = false;
			break;
		}
	}

	if (!free)
		return 0;

	int originalCount = count;

	// Set neccesary bits
	usedBlocks += count + 1;
	while(count-- >= 0)
		setBit(block++);

	// Return number of blocks allocated
	return originalCount + 1;
}

// Frees blocks from a specific memory region
// and returns the number of blocks freed
int pmmFreeRegion(uintptr_t base, size_t size)
{
	int block = base / PMM_BLOCK_SIZE;
	int count = size / PMM_BLOCK_SIZE;
	int originalCount = count;

	// Clear neccesary bits
	usedBlocks -= count + 1;
	while(count-- >= 0)
		clearBit(block++);

	// Return number of blocks freed
	return originalCount + 1;
}


// Allocates one block an returns its address
void* pmmAlloc()
{
	if (freeBlockCount() == 0)
		return 0;

	int block = firstFreeBlock();
	if (block == -1)
		return 0;

	setBit(block);
	usedBlocks++;

	uintptr_t address = block * PMM_BLOCK_SIZE;

	return (void*)address;
}

// Frees the block at the given address
void pmmFree(void* ptr)
{
	int block = (uintptr_t)ptr / PMM_BLOCK_SIZE;
	clearBit(block);
	usedBlocks--;
}


// Allocates a continuous set of blocks
void* pmmAllocContinuous(size_t size)
{
	if (freeBlockCount() < size)
		return 0;

	int block = firstFreeContinuous(size);
	if (block == -1)
		return 0;

	for (uint32_t i = 0; i < size; i++)
		setBit(block + i);

	usedBlocks += size;

	uintptr_t addr = block * PMM_BLOCK_SIZE;
	return (void*)addr;
}

// Frees a continuous set of blocks
void pmmFreeContinuous(void* ptr, size_t size)
{
	int block = (uintptr_t)ptr / PMM_BLOCK_SIZE;

	for (uint32_t i = 0; i < size; i++)
		clearBit(block + i);

	usedBlocks -= size;
}
