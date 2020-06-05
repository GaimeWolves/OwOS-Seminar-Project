#include <memory/heap.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <string.h>

#include <memory/pmm.h>
#include <debug.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

// The header holding information about an allocation
// Resembles a doubly-linked-list of allocations inside a chunk
typedef struct allocation_t
{
	struct allocation_t *prev;
	struct allocation_t *next;

	size_t size; // Size of allocation in bytes
} allocation_t;

// The header at the beginning of a heap chunk
// Resembles a doubly-linked-list of chunks
typedef struct chunk_t
{
	struct chunk_t *next;
	struct chunk_t *prev;

	// The amount of memory blocks this chunk spans
	size_t blocks;

	// Holds the table of allocations of this chunk
	allocation_t *table;
} chunk_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

// Pointer to the initial block of the heap
static chunk_t *heap;

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static inline void moveHeap(chunk_t *new);

static inline uintptr_t ptr(allocation_t *alloc);
static inline uintptr_t allocEnd(allocation_t *alloc);
static inline uintptr_t chunkEnd(chunk_t *chunk);

static void tryLinkChunks(chunk_t *before, chunk_t *after);
static int tryResizeChunk(chunk_t *chunk, size_t size);
static chunk_t* createNewChunk(size_t size);
static chunk_t* extendHeap(size_t size);

static void optimizeChunk(chunk_t *chunk);

static allocation_t* createAllocation(chunk_t *chunk, size_t size);
static void removeAllocation(chunk_t *chunk, allocation_t *allocation);

static allocation_t* allocate(size_t size);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Moves the heap to the new lowest chunk
static inline void moveHeap(chunk_t *new)
{
	heap = new;

	if (heap)
		debug_printf("[HEAP] Moved to %p", (void*)heap);
	else
		debug_print("[HEAP] Heap is empty");
}

// Get address of allocated space
static inline uintptr_t ptr(allocation_t *alloc)
{
	return (uintptr_t)alloc + sizeof(allocation_t);
}

// Get address after an allocation
static inline uintptr_t allocEnd(allocation_t *alloc)
{
	return (uintptr_t)alloc + alloc->size + sizeof(allocation_t);
}

// Get address after a chunk
static inline uintptr_t chunkEnd(chunk_t *chunk)
{
	return (uintptr_t)chunk + chunk->blocks * PMM_BLOCK_SIZE;
}

// Tries to link two adjacent chunks together into one big chunk.
// Gets called after a chunk has been resized to minimize the amount of chunks
static void tryLinkChunks(chunk_t *lower, chunk_t *higher)
{
	if (!lower || !higher)
		return;

	uintptr_t end = chunkEnd(lower);

	// Also adjacent in memory
	if (end == (uintptr_t)higher)
	{
		// Link allocation tables together
		allocation_t *lastAlloc = lower->table;

		if (lastAlloc)
		{
			// Get last allocation in the lower chunks table
			while (lastAlloc->next)
				lastAlloc = lastAlloc->next;

			if (higher->table) higher->table->prev = lastAlloc;
			lastAlloc->next = higher->table;
		}
		else
			lower->table = higher->table;

		// Merge chunks by adjusting removing references to higher chunk 
		if (higher->next) higher->next->prev = lower;
		lower->next = higher->next;
		// Add pages of the higher chunk to the lower
		lower->blocks += higher->blocks;

		debug_printf("[HEAP] Linked chunks @ %p and %p", (void*)lower, (void*)higher);
	}
}

// Tries to resize an existing chunk to be able to hold the needed allocation
static int tryResizeChunk(chunk_t *chunk, size_t size)
{
	uintptr_t chunkPtr = (uintptr_t)chunk;

	// Try to extend to a smaller address
	if (chunkPtr > size) // Prevent integer underflow
	{
		uintptr_t newAddress = chunkPtr - size;
		
		// Rounds to actual blocks automatically
		size_t blocks = pmmAllocRegion(newAddress, size);
		
		if (blocks > 0) // Actually allocated space
		{
			chunk->blocks += blocks;

			// Copy chunk header
			uintptr_t actualAddress = chunkPtr - blocks * PMM_BLOCK_SIZE;
			memcpy((void*)chunkPtr, (void*)actualAddress, sizeof(chunk_t));

			// Relink chain
			if (chunk->prev) chunk->prev->next = chunk->next;
			if (chunk->next) chunk->next->prev = chunk->prev;

			debug_printf("[HEAP] Extended chunk @ %p downwards to %u pages", (void*)actualAddress, chunk->blocks);

			// Try to link together adjacent chunks
			tryLinkChunks(chunk->prev, (chunk_t*)actualAddress);

			if (chunk == heap) // Heap address got relocated
				moveHeap((chunk_t*)actualAddress);

			return 0;
		}
	}

	// Find end of table
	allocation_t *end = chunk->table;
	while(end->next)
		end = end->next;

	uintptr_t endPtr = allocEnd(end);
	uintptr_t blockEnd = chunkEnd(chunk);

	if (endPtr + size > size) // Prevent integer overflow
	{
		uintptr_t newAddress = endPtr + size;
		
		// Rounds to actual blocks automatically
		size_t blocks = pmmAllocRegion(blockEnd, newAddress - blockEnd);

		if (blocks > 0)
		{
			// Adjust chunk size
			chunk->blocks += blocks;

			debug_printf("[HEAP] Extended chunk @ %p upwards to %u pages", (void*)chunk, chunk->blocks);

			// Try to link together adjacent chunks
			tryLinkChunks(chunk, chunk->next);
			
			return 0;
		}
	}

	// No resizing possible
	return 1;
}

// Creates a new chunk
static chunk_t* createNewChunk(size_t size)
{
	// Calculate number of blocks by rounding up
	size_t chunkSize = ((size + sizeof(chunk_t)) + PMM_BLOCK_SIZE - 1) & -PMM_BLOCK_SIZE;
	size_t blocks = chunkSize / PMM_BLOCK_SIZE;

	// Allocate necessary blocks
	void *addr = pmmAllocContinuous(blocks);

	if (!addr)
		return 0; // Out of memory

	// Add new chunk to heap
	chunk_t *newChunk = (chunk_t*)addr;

	// Initialize chunk data
	newChunk->blocks = blocks;
	newChunk->prev = NULL;
	newChunk->next = NULL;
	newChunk->table = NULL;

	// Find spot to fit chunk into
	for (chunk_t *current = heap; current; current = current->next)
	{
		// Insert chunk between current and previous chunk
		if (newChunk < current)
		{
			newChunk->prev = current->prev;
			newChunk->next = current;

			if (current->prev)
				current->prev->next = newChunk;
			else // Block comes before heap address
				moveHeap(newChunk);

			current->prev = newChunk;
		}
		else if (current->next == NULL)
		{
			current->next = newChunk;
			newChunk->prev = current;
			break;
		}
	}

	debug_printf("[HEAP] Created new chunk with %u blocks @ %p", newChunk->blocks, (void*)newChunk);

	return newChunk;
}

// Extends the heap by creating or extending chunks
static chunk_t* extendHeap(size_t size)
{
	// Try resizing one of the chunks
	for (chunk_t *current = heap; current; current = current->next)
	{
		if (tryResizeChunk(current, size))
			continue; // Not resizable

		return current; // Chunk got resized
	}

	// Create a new chunk
	return createNewChunk(size);
}

// Tries to optimize the size of the chunk
static void optimizeChunk(chunk_t *chunk)
{
	// Case 1: Chunk is empty
	if (!chunk->table)
	{
		if (chunk == heap) // Heap got relocated
			moveHeap(chunk->next);

		// Free chunk
		pmmFreeContinuous(chunk, chunk->blocks);

		// Relink chunk list
		if (chunk->next) chunk->next->prev = chunk->prev;
		if (chunk->prev) chunk->prev->next = chunk->next;
	
		debug_printf("[HEAP] Deleted chunk @ %p", (void*)chunk);
	}
	else
	{
		allocation_t *current = chunk->table;

		// Case 2: Free space before first allocation
		// If there are free blocks between the chunk header and the first allocation
		// the chunk header will be moved to the new block

		size_t space = (uintptr_t)current - ((uintptr_t)chunk + sizeof(chunk_t));
		size_t skipBlocks = space / PMM_BLOCK_SIZE; // Blocks to move forward
		uintptr_t newAddress = (uintptr_t)chunk + skipBlocks * PMM_BLOCK_SIZE;

		// Move chunk to new address
		if (skipBlocks > 0)
		{
			if (chunk == heap) // Heap got relocated
				moveHeap((chunk_t*)newAddress);

			//Copy information struct of the chunk to new location
			memcpy((void*)newAddress, (void*)chunk, sizeof(chunk_t));
			//Signal PMM to free the blocks
			pmmFreeContinuous(chunk, skipBlocks);
			//Set pointer to the new location
			chunk = (chunk_t*)newAddress;

			// Update addresses of neighbour chunks
			if (chunk->next) chunk->next->prev = chunk;
			if (chunk->prev) chunk->prev->next = chunk;

			chunk->blocks -= skipBlocks;

			debug_printf("[HEAP] Shrunk chunk @ %p upwards to %u blocks.", (void*)chunk, chunk->blocks);
		}

		// Case 3: Free space between allocations
		// If there are free blocks between allocations and space for a new chunk header
		// the chunk will be split in two and a new chunk header will be placed at the
		// beginning of the the block where the next allocation lies
		
		// May change while optimizing
		chunk_t *currentChunk = chunk;

		allocation_t *next = current->next;
		while(next)
		{
			uintptr_t currentPtr = allocEnd(current);
			size_t currentBlock = (uintptr_t)currentPtr / PMM_BLOCK_SIZE;
			size_t nextBlock = (uintptr_t)next / PMM_BLOCK_SIZE;
			size_t blockDifference = nextBlock - currentBlock;

			// Atleast one free block between allocations
			if (blockDifference > 1)
			{
				// Space for a new chunk header
				if ((uintptr_t)next - (nextBlock * PMM_BLOCK_SIZE) >= sizeof(chunk_t))
				{
					// Free unnecessary blocks
					pmmFreeContinuous((void*)((currentBlock + 1) * PMM_BLOCK_SIZE), blockDifference - 1);

					// Find out number of remaining blocks
					allocation_t *end = next;
					while(end->next)
						end = end->next;
					size_t endPtr = (uintptr_t)end + end->size + sizeof(allocation_t);
					size_t remBlocks = (endPtr - nextBlock * PMM_BLOCK_SIZE) / PMM_BLOCK_SIZE + 1;

					// Create new chunk and relink everything
					chunk_t *newChunk = (chunk_t*)(nextBlock * PMM_BLOCK_SIZE);
					newChunk->blocks = remBlocks;
					newChunk->table = next;
					
					// Relink chunk chain
					newChunk->prev = currentChunk;
					newChunk->next = currentChunk->next;
					
					if (newChunk->next)
						newChunk->next->prev = newChunk;

					currentChunk->next = newChunk;
					currentChunk->blocks = (currentPtr - (uintptr_t)currentChunk) / PMM_BLOCK_SIZE + 1;

					// Cut allocation chain
					current->next = NULL;
					next->prev = NULL;

					debug_printf("[HEAP] Cut chunk @ %p between %p and %p. Next @ %p",
						(void*)currentChunk,
						(void*)currentPtr,
						(void*)next,
						(void*)newChunk
					);

					// Set current chunk to new chunk
					currentChunk = newChunk;
				}
			}

			current = next;
			next = current->next;
		}

		// Case 4: Free space after last allocation
		// If there are free blocks after a chunk they will be freed

		uintptr_t end = chunkEnd(currentChunk);
		uintptr_t last = allocEnd(current);
		uintptr_t remBlocks = (end - last) / PMM_BLOCK_SIZE;

		if (remBlocks)
			debug_printf("[HEAP] Shrunk chunk %p downwards %u blocks", (void*)currentChunk, remBlocks);

		// Delete redundant blocks
		while (remBlocks--)
			pmmFree((void*)((uintptr_t)chunk + --currentChunk->blocks * PMM_BLOCK_SIZE));
	}
}

// Tries to create an allocation inside the specified chunk
static allocation_t* createAllocation(chunk_t *chunk, size_t size)
{
	// Chunk is too small to hold allocation
	if (chunk->blocks * PMM_BLOCK_SIZE - sizeof(chunk_t) < size)
		return 0;

	allocation_t *alloc = NULL;

	allocation_t *before = NULL;
	allocation_t *after = NULL;

	// Chunk is empty
	if (!chunk->table)
	{
		// Allocate directly after the chunk header
		uintptr_t addr = (uintptr_t)chunk + sizeof(chunk_t);
		alloc = (allocation_t*)addr;

		chunk->table = alloc;
	}
	else
	{
		allocation_t *current = chunk->table;
		allocation_t *next = current->next;

		// Check space between allocations
		while(next)
		{
			uintptr_t end = allocEnd(current);
			size_t available = (uintptr_t)next - end;

			if (available >= size + sizeof(allocation_t))
			{
				// Create allocation
				alloc = (allocation_t*)end;
				before = current;
				after = next;
				break;
			}

			// Move to next allocation
			current = next;
			next = current->next;
		}

		if (!alloc)
		{
			// Check space from the last allocation to the end of the chunk
			uintptr_t end = chunkEnd(chunk);
			uintptr_t last = allocEnd(current);
		
			size_t available = end - last;

			if (available >= size + sizeof(allocation_t))
			{
				alloc = (allocation_t*)last;
				before = current;
			}
		}
	}

	if (!alloc) // No space in chunk
		return 0;

	// Insert the allocation into list
	if (before) before->next = alloc;
	alloc->prev = before;

	if (after) after->prev = alloc;
	alloc->next = after;

	// Return the allocation
	alloc->size = size;
	return alloc;
}

// Removes an allocation and tries to optimize the heap afterwards
static void removeAllocation(chunk_t *chunk, allocation_t *allocation)
{
	// Relink chunk header to allocation table
	if (chunk->table == allocation)
		chunk->table = allocation->next;

	// Relink allocations before and after the deleted one
	if (allocation->next) allocation->next->prev = allocation->prev;
	if (allocation->prev) allocation->prev->next = allocation->next;

	// Possibly optimize the chunk size
	optimizeChunk(chunk);
}

// Tries to allocate the amount of bytes
// If there is no space in the current heap the heap will be extended accordingly
static allocation_t* allocate(size_t size)
{
	for (chunk_t *chunk = heap; chunk != NULL; chunk = chunk->next)
	{
		allocation_t *alloc = createAllocation(chunk, size);
		if (alloc)
			return alloc;
	}

	chunk_t* newChunk = extendHeap(size + sizeof(allocation_t));

	// No memory available
	if (!newChunk)
		return 0;

	if (!heap) // Empty heap
	{
		heap = newChunk;
		debug_printf("[HEAP] Created heap @ %p", (void*)heap);
	}
	
	return createAllocation(newChunk, size);
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Allocates the neded space
// Return zero if the system is out of memory
void* kmalloc(size_t size)
{
	allocation_t *alloc = allocate(size);
	
	if (!alloc)
		debug_print("Allocation failed: Out of memory");

	return (void*)ptr(alloc);
}

// Allocates the needed space and memsets it to zero
void* kzalloc(size_t size)
{
	void* alloc = kmalloc(size);
	
	if (alloc)
		memset(alloc, 0, size);

	return alloc;
}

// Allocates space for an array
void* kmalloc_array(size_t n, size_t size)
{
	size_t actualSize = n * size;
	return kmalloc(actualSize);
}

// Allocates space for an array and memsets it to zero
void* kcalloc(size_t n, size_t size)
{
	void* alloc = kmalloc_array(n, size);

	if (alloc)
		memset(alloc, 0, n * size);

	return alloc;
}

// Allocates space for the string and copies it
char* kstrdup(const char *s)
{
	size_t len = strlen(s) + 1; // Include null-byte
	void* alloc = kmalloc(len);

	if (alloc)
		memcpy(alloc, (void*)s, len);

	return (char*)alloc;
}


// Allocates space for the string and copies it
// Copies at most max chars
char* kstrndup(const char *s, size_t max)
{
	size_t len = strlen(s);
	if (len > max)
		len = max;
	
	char* alloc = (char*)kmalloc(len + 1);
	
	if (alloc)
	{
		memcpy((void*)alloc, (void*)s, len);
		alloc[len] = '\0';
	}

	return (char*)alloc;
}

// Allocates space for the memory region and copies
// the memory from the source
void* kmemdup(const void *src, size_t len)
{
	void *alloc = kmalloc(len);

	if (alloc)
		memcpy(alloc, src, len);

	return alloc;
}

// Frees the specified heap allocation
void kfree(const void *objp)
{
	for (chunk_t *chunk = heap; chunk != NULL; chunk = chunk->next)
	{
		for (allocation_t *alloc = chunk->table; alloc != NULL; alloc = alloc->next)
		{
			if ((uintptr_t)alloc + sizeof(allocation_t) == (uintptr_t)objp)
			{
				removeAllocation(chunk, alloc);
				return;
			}
		}
	}
}
