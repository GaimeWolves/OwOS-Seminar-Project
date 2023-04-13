#include <vfs/fat32.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <ctype.h>
#include <string.h>

#include <memory/heap.h>
#include <hal/atapio.h>
#include <vfs/vfs.h>
#include <debug.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

// Special filename parsing flags
#define LOWERCASE_EXT  0x08
#define LOWERCASE_BASE 0x04

// Attribute flags
#define READ_ONLY 0x01
#define HIDDEN    0x02
#define SYSTEM    0x04
#define VOLUME_ID 0x08
#define DIRECTORY 0x10
#define ARCHIVE   0x20

// Constants
#define LFN_ENTRY     0x0F       // LFN entry identifier
#define UNUSED        0xE5       // Unused directory entry
#define FAT_MASK      0x0FFFFFFF // Mask for FAT entries
#define CLUSTER_LIMIT 0x0FFFFFF7 // First invalid cluster number
#define READ          false
#define WRITE         true

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

// Extended boot parameter block
typedef struct ebpb_t
{
	uint32_t sectorsPerFAT;
	uint16_t flags;
	uint16_t version;
	uint32_t rootcluster;
	uint16_t fsinfoSector;
	uint16_t bootbackupSector;
	uint8_t reserved1[12];
	uint8_t drive;
	uint8_t reserved2;
	uint8_t signature;
	uint32_t serial;
	char label[11];
	char identifier[8];
	uint8_t bootcode[420];
	uint16_t bootSignature;
} __attribute__((packed)) ebpb_t;

// Boot parameter block
typedef struct bpb_t
{
	uint8_t jumpInstr[3];
	char identifier[8];
	uint16_t bytesPerSector;
	uint8_t sectorsPerCluster;
	uint16_t reservedSectors;
	uint8_t numberOfFATs;
	uint16_t directoryEntries;
	uint16_t sectorCount;
	uint8_t mediaType;
	uint16_t unused;
	uint16_t sectorsPerTrack;
	uint16_t numberOfHeads;
	uint32_t hiddenSectors;
	uint32_t largeSectorCount;
	ebpb_t ebpb;
} __attribute__((packed)) bpb_t;

// FSInfo struct
typedef struct fsinfo_t
{
	uint32_t leadSignature;
	uint8_t reserved1[480];
	uint32_t signature;
	uint32_t lastFreeCluster;
	uint32_t firstFreeCluster;
	uint8_t reserved2[12];
	uint32_t trailSignature;
} __attribute__((packed)) fsinfo_t;

// Metadata structure for FAT32
typedef struct fat32_metadata_t
{
	bpb_t *bpb;
	fsinfo_t *fsinfo;

	uint32_t bytesPerCluster;
	uint32_t firstDataSector;
	uint32_t firstFATSector;
} fat32_metadata_t;

// Directory entry structure
typedef struct dir_entry_t
{
	char name[8];
	char extension[3];
	uint8_t attributes;
	uint8_t nametype;

	struct
	{
		uint8_t tenths;
		uint16_t hour : 5;
		uint16_t minute : 6;
		uint16_t second : 5;
		uint16_t year : 7;
		uint16_t month : 4;
		uint16_t day : 5;
	} __attribute__((packed)) creationTime;

	struct
	{
		uint16_t year : 7;
		uint16_t month : 4;
		uint16_t day : 5;
	} __attribute__((packed)) accessDate;

	uint16_t clusterHigh;

	struct
	{
		uint16_t hour : 5;
		uint16_t minute : 6;
		uint16_t second : 5;
		uint16_t year : 7;
		uint16_t month : 4;
		uint16_t day : 5;
	} __attribute__((packed)) modificationTime;

	uint16_t clusterLow;
	uint32_t size;
} __attribute__((packed)) dir_entry_t;

// Long file name structure
typedef struct lfn_entry_t
{
	uint8_t index;
	uint16_t wchar1[5];
	uint8_t signature;
	uint8_t type;
	uint8_t checksum;
	uint16_t wchar2[6];
	uint16_t unused;
	uint16_t wchar3[2];
} __attribute__((packed)) lfn_entry_t;

// Represents a cluster chain with a linked list
typedef struct cluster_chain_t
{
	uint32_t value;               // Cluster value
	uint32_t index;               // Cluster index
	struct cluster_chain_t *next; // Next cluster
} cluster_chain_t;

// Doubly-linked list to temporairly save filename
typedef struct lfn_chain_t
{
	uint8_t index;
	char part[13];
	uint8_t checksum;
	struct lfn_chain_t *prev;
	struct lfn_chain_t *next;
} lfn_chain_t;

// Linked list to temporairly save directory entries
typedef struct dir_chain_t
{
	char *fullname;
	dir_entry_t entry;
	struct dir_chain_t *next;
} dir_chain_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static inline uint32_t cluster(dir_entry_t *entry);
static uint8_t calcChecksum(const char *name);

static void deleteClusterChain(cluster_chain_t *chain);
static void deleteDirectoryChain(dir_chain_t *chain);
static void deleteLFNChain(lfn_chain_t *chain);

static uint32_t getClusterSector(mountpoint_t *metadata, uint32_t cluster);
static cluster_chain_t *getChain(mountpoint_t *metadata, uint32_t first);
static size_t getClusterCount(cluster_chain_t *chain);
static int doClusterOperation(void* buf, mountpoint_t *metadata, cluster_chain_t *chain, uint32_t offset, bool write);
static int removeCluster(cluster_chain_t *chain, mountpoint_t *metadata);
static cluster_chain_t *addCluster(cluster_chain_t *chain, mountpoint_t *metadata);
static cluster_chain_t *createChain(mountpoint_t *metadata, size_t size);
static int shrinkChain(mountpoint_t *metadata, cluster_chain_t *chain, size_t newSize);

static dir_chain_t *parseDirectory(file_desc_t *file);
static char *getFullName(dir_entry_t *entry, lfn_chain_t *lfn);
static void addLFNEntry(lfn_chain_t **chain, lfn_entry_t *entry);
static char *parseShortName(dir_entry_t *entry);
static char *parseLongName(dir_entry_t *entry, lfn_chain_t *lfn);
static int removeDirectoryEntry(file_desc_t *dir, char *origName, uint32_t inode);
static int addDirectoryEntry(file_desc_t *dir, file_desc_t *file);
static int updateDirectoryEntry(file_desc_t *dir, file_desc_t *file, char *origName);
static char* toShortName(char *name);
static dir_entry_t toDirEntry(file_desc_t *file);
static lfn_entry_t toLFNEntry(file_desc_t *file, int index);
static int initDirectory(file_desc_t *dir);

static file_desc_t *createFile(file_desc_t *root, dir_chain_t *direntry);

static size_t doFileOperation(file_desc_t *file, size_t offset, size_t size, char *buf, bool write);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Convert the clusterLow and clusterHigh to the real cluster number
static inline uint32_t cluster(dir_entry_t *entry)
{
	return (uint32_t)entry->clusterLow | ((uint32_t)entry->clusterHigh << 16);
}

// Calculates the cecksum present on long file name entries
// from the 8.3 filename
static uint8_t calcChecksum(const char *name)
{
	uint8_t *filename = (uint8_t*)name;

	uint8_t sum = 0;

	for (int i = 11; i > 0; i--)
		sum = ((sum & 1) << 7) + (sum >> 1) + *filename++;

	return sum;
}

// Frees the used memory of the cluster chain
static void deleteClusterChain(cluster_chain_t *chain)
{
	cluster_chain_t *current = chain;

	while(current)
	{
		// Free the cluster chain struct
		cluster_chain_t *next = current->next;
		kfree(current);
		current = next;
	}
}

// Frees the used memory of the directory chain
static void deleteDirectoryChain(dir_chain_t *chain)
{
	dir_chain_t *current = chain;

	while(current)
	{
		// Free the cluster chain struct
		dir_chain_t *next = current->next;
		kfree(current->fullname);
		kfree(current);
		current = next;
	}
}

// Frees the used memory of the lfn chain
static void deleteLFNChain(lfn_chain_t *chain)
{
	if (!chain)
		return;

	lfn_chain_t *current = chain;

	while(current->prev)
		current = current->prev;

	while(current)
	{
		// Free the cluster chain struct
		lfn_chain_t *next = current->next;
		kfree(current);
		current = next;
	}
}

// Calculates the first sector the cluster specifies
static uint32_t getClusterSector(mountpoint_t *metadata, uint32_t cluster)
{
	fat32_metadata_t *data = (fat32_metadata_t*)metadata->metadata;
	
	return ((cluster - 2) * data->bpb->sectorsPerCluster) + data->firstDataSector;
}

// Reads the cluster chain specified by its first cluster
static cluster_chain_t *getChain(mountpoint_t *metadata, uint32_t first)
{
	fat32_metadata_t *data = ((fat32_metadata_t*)metadata->metadata);

	cluster_chain_t *chain = kzalloc(sizeof(cluster_chain_t));
	chain->index = first;

	cluster_chain_t *current = chain;

	while (true)
	{
		// Calculate offset into FAT table
		uint8_t table[data->bpb->bytesPerSector];                                      // Subset of FAT table
		uint32_t offset = current->index * 4;                                          // Byte offset
		uint32_t sector = data->firstFATSector + (offset / data->bpb->bytesPerSector); // Sector to read
		uint32_t sectorOffset = offset % data->bpb->bytesPerSector;                    // Sector offset

		// Read a sector from the FAT table
		if (ataRead((void*)table, sector, 1, metadata->partition->device))
		{
			debug_set_color(0x0C, 0x00);
			debug_printf("Couldn't read cluster value %u", current->index);
			debug_set_color(0x0F, 0x00);
			deleteClusterChain(chain);
			return NULL;
		}

		// Read the cluster value and mask upper 4 bits
		uint32_t value = *(uint32_t*)&table[sectorOffset] & FAT_MASK;
		current->value = value;

		// Check if the cluster is valid
		if (value > 0x1 && value < CLUSTER_LIMIT)
		{
			// Add new cluster to chain
			cluster_chain_t *next = kzalloc(sizeof(cluster_chain_t));
			current->next = next;
			next->index = value;
			next->next = NULL;
			current = next;
		}
		else // End of chain
			break;
	}

	return chain;
}

// Remove the last cluster from the chain
static int removeCluster(cluster_chain_t *chain, mountpoint_t *metadata)
{
	fat32_metadata_t *data = ((fat32_metadata_t*)metadata->metadata);
	
	// Find last and previous cluster
	cluster_chain_t *last = chain;
	cluster_chain_t *prev = NULL;
	for(; last->next; prev = last, last = last->next);

	uint8_t table[data->bpb->bytesPerSector];
	
	// Calcluate offset of the last cluster
	uint32_t offset = last->index * 4;
	uint32_t sector = data->firstFATSector + (offset / data->bpb->bytesPerSector);
	uint32_t sectorOffset = offset % data->bpb->bytesPerSector;

	// Read a sector from the FAT table
	if (ataRead((void*)table, sector, 1, metadata->partition->device))
		return EOF;

	// Set the cluster's value to zero (free cluster)
	*(uint32_t*)&table[sectorOffset] = 0;

	// Write the modified sector back into the FAT table
	if (ataWrite((void*)table, sector, 1, metadata->partition->device))
		return EOF;

	// Set the previous cluster to be the EOC
	if (prev)
	{
		prev->next = NULL;
		prev->value = 0x0FFFFFFF; // EOC value

		// Calculate its offset and update the FAT
		offset = prev->index * 4;
		sector = data->firstFATSector + (offset / data->bpb->bytesPerSector);
		sectorOffset = offset % data->bpb->bytesPerSector;

		if (ataRead((void*)table, sector, 1, metadata->partition->device))
			return EOF;

		*(uint32_t*)&table[sectorOffset] = 0x0FFFFFFF;

		if (ataWrite((void*)table, sector, 1, metadata->partition->device))
			return EOF;
	}

	kfree(last);

	return 0;
}

// Add a cluster to the end of a cluster chain
static cluster_chain_t *addCluster(cluster_chain_t *chain, mountpoint_t *metadata)
{
	fat32_metadata_t *data = ((fat32_metadata_t*)metadata->metadata);

	// Find last cluster in the chain
	cluster_chain_t *last = chain;
	if (last)
		for (; last->next; last = last->next);

	uint8_t table[data->bpb->bytesPerSector];
	uint32_t count = data->bpb->sectorCount == 0 ? data->bpb->largeSectorCount : data->bpb->sectorCount;
	uint32_t sector = 0, offset = 0, newSector = 0;

	cluster_chain_t *new = NULL;

	// Iterate through the FAT table
	for (uint32_t cluster = 2; cluster < count / data->bpb->sectorsPerCluster; cluster++)
	{
		offset = cluster * 4;
		newSector = data->firstFATSector + (offset / data->bpb->bytesPerSector);

		// Check if we crossed a sector boundary
		if (newSector != sector)
		{
			sector = newSector;
			
			// Read the new sector
			if (ataRead((void*)table, sector, 1, metadata->partition->device))
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Couldn't read fat sector");
				debug_set_color(0x0F, 0x00);
				return NULL;
			}
		}

		// Calculate the current offset into the FAT table and read the value
		uint32_t sectorOffset = offset % data->bpb->bytesPerSector;
		uint32_t value = *(uint32_t*)&table[sectorOffset] & FAT_MASK;

		if (value == 0) // Free cluster
		{
			// Set the cluster's value to be EOC
			*(uint32_t*)&table[sectorOffset] = 0x0FFFFFFF;

			// Allocate a new cluster link to add to the chain
			new = kzalloc(sizeof(cluster_chain_t));
			new->value = 0x0FFFFFFF;
			new->index = cluster;

			// Write the modified FAT table
			if (ataWrite((void*)table, sector, 1, metadata->partition->device))
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Couldn't write new cluster");
				debug_set_color(0x0F, 0x00);
				kfree(new);
				return NULL;
			}

			if (!last) // No chain to update
			{
				chain = new;
				break;
			}

			// Update the previous last cluster to point to the new EOC
			last->value = cluster;
			last->next = new;

			offset = last->index * 4;
			sector = data->firstFATSector + (offset / data->bpb->bytesPerSector);
			sectorOffset = offset % data->bpb->bytesPerSector;

			if (ataRead((void*)table, sector, 1, metadata->partition->device))
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Couldn't read fat sector");
				debug_set_color(0x0F, 0x00);
				kfree(new);
				return NULL;
			}

			*(uint32_t*)&table[sectorOffset] = last->value;

			if (ataWrite((void*)table, sector, 1, metadata->partition->device))
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Couldn't write new cluster");
				debug_set_color(0x0F, 0x00);
				kfree(new);
				return NULL;
			}

			break;
		}
	}

	if (new)
	{
		// Clear new cluster as complications can occur with directory parsing
		char *cluster = kzalloc(data->bytesPerCluster);
		doClusterOperation(cluster, metadata, chain, getClusterCount(chain) - 1, WRITE);
		kfree(cluster);
	}

	return chain;
}

// Create a cluster chain able to hold a file of the specified size
static cluster_chain_t *createChain(mountpoint_t *metadata, size_t size)
{
	if (size == 0)
		return NULL;

	fat32_metadata_t *data = (fat32_metadata_t*)metadata->metadata;

	// Round size up to the next multiple of the bytes per cluster
	size_t count = (size + data->bytesPerCluster - 1) / data->bytesPerCluster - 1;
	cluster_chain_t *chain = addCluster(NULL, metadata);

	if (!chain)
		return NULL;

	// Continuously call addCluster to build up the cluster chain
	for (; count > 0; count--)
	{
		if (!addCluster(chain, metadata))
		{
			shrinkChain(metadata, chain, 0); // Delete chain again
			return NULL;
		}
	}

	return chain;
}

// Shrinks the size of the cluster chain to the minimum amount of clusters to hold a file of size newSize
static int shrinkChain(mountpoint_t *metadata, cluster_chain_t *chain, size_t newSize)
{
	fat32_metadata_t *data = (fat32_metadata_t*)metadata->metadata;
	
	size_t currentCount = getClusterCount(chain);
	size_t newCount = (newSize + data->bytesPerCluster - 1) / data->bytesPerCluster;

	// Continuously call removeCluster to shrink the cluster chain
	for (size_t i = currentCount; i > newCount; i--)
		if(removeCluster(chain, metadata))
			return EOF;

	return 0;
}

// Get the amount of clusters of the chain
static size_t getClusterCount(cluster_chain_t *chain)
{
	size_t len = 0;

	for (cluster_chain_t *current = chain; current != NULL; current = current->next, len++);

	return len;
}

// Writes/Reads the contents of the buffer/cluster into the clustee/buffer
static int doClusterOperation(void* buf, mountpoint_t *metadata, cluster_chain_t *chain, uint32_t offset, bool write)
{
	// Past the last cluster
	if (offset >= getClusterCount(chain))
		return EOF;

	// Iterate through chain
	cluster_chain_t *current = chain;
	for (; offset > 0; offset--)
		current = current->next;

	// Read the cluster into memory
	fat32_metadata_t *data = (fat32_metadata_t*)metadata->metadata;

	if (write)
	{
		if (ataWrite(buf, getClusterSector(metadata, current->index), data->bpb->sectorsPerCluster, metadata->partition->device))
		{
			debug_set_color(0x0C, 0x00);
			debug_printf("Couldn't write cluster %u", current->index);
			debug_set_color(0x0F, 0x00);
			return -2;
		}
	}
	else
	{
		if (ataRead(buf, getClusterSector(metadata, current->index), data->bpb->sectorsPerCluster, metadata->partition->device))
		{
			debug_set_color(0x0C, 0x00);
			debug_printf("Couldn't read cluster %u", current->index);
			debug_set_color(0x0F, 0x00);
			return -2;
		}
	}

	return 0;
}

// Reads the complete directory into a traversable chain
static dir_chain_t *parseDirectory(file_desc_t *file)
{
	cluster_chain_t *chain = getChain(file->mount, file->inode);

	fat32_metadata_t *metadata = (fat32_metadata_t*)file->mount->metadata;

	uint8_t* buf = kmalloc(metadata->bytesPerCluster);
	uint32_t offset = metadata->bytesPerCluster;
	int cluster = -1;
	
	lfn_chain_t *lfn = NULL;
	dir_chain_t *dir = NULL;
	dir_chain_t *current = NULL;

	// Read all directory entries
	while(true)
	{
		// Did we cross a cluster boundary
		if (offset >= metadata->bytesPerCluster)
		{
			// Update the counters
			offset = 0;
			cluster++;

			// Read the next cluster into the buffer
			int ret = doClusterOperation(buf, file->mount, chain, cluster, READ);
			if (ret == EOF)
				break;

			if (ret)
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Directory is corrupted or couldn't be read");
				debug_set_color(0x0F, 0x00);

				deleteLFNChain(lfn);
				kfree(buf);
				deleteClusterChain(chain);

				return dir;
			}
		}

		if (buf[offset] == 0) // No more entries
			break;
		else if (buf[offset] == UNUSED)
		{
			offset += sizeof(dir_entry_t);
			continue;
		}
		else if (buf[offset + 11] == LFN_ENTRY)
		{
			// Add the lfn entry to the current chain
			lfn_entry_t *lfnEntry = (lfn_entry_t*)&buf[offset];
			addLFNEntry(&lfn, lfnEntry);
		}
		else // Normal directory entry
		{
			dir_entry_t *entry = (dir_entry_t*)&buf[offset];

			// Parse the entry's full name
			char *fullname = getFullName(entry, lfn);
			deleteLFNChain(lfn);
			lfn = NULL;

			// Create new chain entry
			dir_chain_t *next = kzalloc(sizeof(dir_chain_t));
			next->entry = *entry;
			next->fullname = fullname;

			// Link new entry
			if (current)
				current->next = next;
			else
				dir = next;

			current = next;
		}

		// Go to the next entry
		offset += sizeof(dir_entry_t);
	}

	// Free allocated memory
	deleteLFNChain(lfn);
	deleteClusterChain(chain);
	kfree(buf);

	return dir;
}

// Removes a directory entry from a directory.
// Identifies the entry by checking checksum and inode values
static int removeDirectoryEntry(file_desc_t *dir, char *origName, uint32_t inode)
{
	// Calculate 8.3 name and checksum
	char *shortName = toShortName(origName);
	uint8_t checksum = calcChecksum(shortName);

	fat32_metadata_t *metadata = (fat32_metadata_t*)dir->mount->metadata;
	cluster_chain_t *chain = getChain(dir->mount, dir->inode);
	
	uint8_t* buf = kmalloc(metadata->bytesPerCluster);
	int currentCluster = -1;
	uint32_t offset = metadata->bytesPerCluster;

	int beginCluster = -1, beginOffset = 0; // Offset of the first fitting lfn entry

	while(true)
	{
		// Did we cross a cluster boundary
		if (offset >= metadata->bytesPerCluster)
		{
			// Update the counters
			offset = 0;
			currentCluster++;

			// Read the next cluster into the buffer
			int ret = doClusterOperation(buf, dir->mount, chain, currentCluster, READ);
			if (ret == EOF)
				break;

			if (ret)
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Directory is corrupted or couldn't be read");
				debug_set_color(0x0F, 0x00);

				kfree(buf);
				deleteClusterChain(chain);
				kfree(shortName);
				return EOF;
			}
		}

		if (buf[offset] == 0) // No more entries
			break;
		else if (buf[offset] == UNUSED)
		{
			offset += sizeof(dir_entry_t);
			continue;
		}
		else if (buf[offset + 11] == LFN_ENTRY)
		{
			lfn_entry_t *lfnEntry = (lfn_entry_t*)&buf[offset];

			// Check if it is the corresponding lfn entry
			if (checksum == lfnEntry->checksum)
			{
				if (beginCluster == -1)
				{
					// Save the beginning of the area to clear
					beginCluster = currentCluster;
					beginOffset = offset;
				}
			}
		}
		else // Normal directory entry
		{
			dir_entry_t *entry = (dir_entry_t*)&buf[offset];

			// Check if the checksum and inode match up
			if (calcChecksum(entry->name) == checksum && inode == cluster(entry))
			{
				if (beginCluster != -1)
				{
					// Set all entries to UNUSED
					for (int i = currentCluster; i >= beginCluster; i--)
					{
						doClusterOperation(buf, dir->mount, chain, i, READ);

						int start = i == beginCluster ? beginOffset : 0;
						int end = i == currentCluster ? offset : metadata->bytesPerCluster;

						for (int j = start; j <= end; j += sizeof(dir_entry_t))
							buf[j] = UNUSED;

						doClusterOperation(buf, dir->mount, chain, i, WRITE);
					
						kfree(buf);
						deleteClusterChain(chain);
						kfree(shortName);
						return 0;
					}
				}
				else
				{
					// Set only the directory entry to UNUSED
					buf[offset] = UNUSED;
					doClusterOperation(buf, dir->mount, chain, currentCluster, WRITE);
					
					kfree(buf);
					deleteClusterChain(chain);
					kfree(shortName);
					return 0;
				}
			}

			beginCluster = -1;
			beginOffset = 0;
		}
		
		offset += sizeof(dir_entry_t);
	}

	kfree(buf);
	deleteClusterChain(chain);
	kfree(shortName);
	return EOF;
}

// Adds a directory entry to a directory
static int addDirectoryEntry(file_desc_t *dir, file_desc_t *file)
{
	char *shortName = toShortName(file->name);

	int count = ((strlen(file->name) + 1) + 13 - 1) / 13; // Needed LFN entry count

	fat32_metadata_t *metadata = (fat32_metadata_t*)dir->mount->metadata;
	cluster_chain_t *chain = getChain(dir->mount, dir->inode);
	
	uint8_t* buf = kmalloc(metadata->bytesPerCluster);
	int currentCluster = -1;
	uint32_t offset = metadata->bytesPerCluster;

	int beginCluster = -1, beginOffset = 0; // Offset of the first empty entry
	int numEntries = 0;                     // Number of free entries

	while(true)
	{
		// Read next cluster into buffer
		if (offset >= metadata->bytesPerCluster)
		{
			offset = 0;
			currentCluster++;

			// Try to allocate a new cluster for the directory
			if ((size_t)currentCluster >= getClusterCount(chain))
			{
				if (!addCluster(chain, dir->mount))
				{
					debug_set_color(0x0C, 0x00);
					debug_print("Couldn't allocate Cluster! Partition may be full!");
					debug_set_color(0x0F, 0x00);

					kfree(buf);
					deleteClusterChain(chain);
					kfree(shortName);
					return EOF;
				}
			}

			int ret = doClusterOperation(buf, dir->mount, chain, currentCluster, READ);
			if (ret == EOF)
				break;

			if (ret)
			{
				debug_set_color(0x0C, 0x00);
				debug_print("Directory is corrupted or couldn't be read");
				debug_set_color(0x0F, 0x00);

				kfree(buf);
				deleteClusterChain(chain);
				kfree(shortName);
				return EOF;
			}
		}

		// Free entry
		if (buf[offset] == 0 || buf[offset] == UNUSED)
		{
			if (beginCluster == -1)
			{
				// Save the first free entry and reset entry count
				beginCluster = currentCluster;
				beginOffset = offset;
				numEntries = 0;
			}

			numEntries++; // Update free entry count
		}
		else // Used entry
			beginCluster = -1; // Reset free entry count

		// Do we have enough free entries to fit the new one
		if (numEntries == count + 1)
		{
			currentCluster = beginCluster;
			offset = beginOffset;

			doClusterOperation(buf, dir->mount, chain, beginCluster, READ);

			// Continuously write the needed entries
			for(int i = 0; i < numEntries; i++, offset += sizeof(dir_entry_t))
			{
				// Did we cross a cluster boundary
				if (offset >= metadata->bytesPerCluster)
				{
					// Write the modified cluster and read the next one
					doClusterOperation(buf, dir->mount, chain, currentCluster, WRITE);
					offset = 0;
					currentCluster++;
					doClusterOperation(buf, dir->mount, chain, currentCluster, READ);
				}

				if (i == count) // Write 8.3 entry
				{
					dir_entry_t entry = toDirEntry(file);
					*(dir_entry_t*)&buf[offset] = entry;
				}
				else            // Write LFN entry
				{
					lfn_entry_t entry = toLFNEntry(file, count - i);
					*(lfn_entry_t*)&buf[offset] = entry;
				}
			}

			// Write the last cluster
			doClusterOperation(buf, dir->mount, chain, currentCluster, WRITE);

			kfree(buf);
			deleteClusterChain(chain);
			kfree(shortName);
			return 0;
		}

		offset += sizeof(dir_entry_t);
	}

	kfree(buf);
	deleteClusterChain(chain);
	kfree(shortName);
	return EOF;
}

// Updates a directory entry with the new data (old name is necessary for identification)
static int updateDirectoryEntry(file_desc_t *dir, file_desc_t *file, char *origName)
{
	// Rewrite entry
	if(removeDirectoryEntry(dir, origName, file->inode))
		return EOF;

	if(addDirectoryEntry(dir, file))
		return EOF;

	return 0;
}

// Converts the data of the file into an 8.3 entry
static dir_entry_t toDirEntry(file_desc_t *file)
{
	dir_entry_t entry;
	memset(&entry, 0, sizeof(dir_entry_t));

	// Write 8.3 name
	char *name = toShortName(file->name);
	memcpy(entry.name, name, 11);
	kfree(name);

	// Write attributes
	entry.attributes = file->flags & FS_DIRECTORY ? DIRECTORY : 0;
	entry.clusterHigh = file->inode >> 16;
	entry.clusterLow = file->inode & 0xFFFF;
	entry.size = file->length;

	return entry;
}

// Converts the data of the file to the LFN entry at the specified index
static lfn_entry_t toLFNEntry(file_desc_t *file, int index)
{
	// Calculate checksum
	char* shortName = toShortName(file->name);
	uint8_t checksum = calcChecksum(shortName);
	kfree(shortName);

	lfn_entry_t entry;
	memset(&entry, 0, sizeof(lfn_entry_t));

	// Initialize UCS-2 chars to 0xFFFF (unused)
	for(int i = 0; i < 13; i++)
	{
		if (i >= 11)
			entry.wchar3[i - 11] = 0xFFFF;
		else if (i >= 5)
			entry.wchar2[i - 5] = 0xFFFF;
		else
			entry.wchar1[i] = 0xFFFF;
	}

	// Set the attributes
	entry.index = (uint8_t)index;
	entry.checksum = checksum;
	entry.signature = 0xF;

	bool last = false;

	// Write a substring of the filename into the lfn entry
	for (int i = 0; i < 13; i++)
	{
		uint16_t value = (uint16_t)file->name[(index - 1) * 13 + i];

		if (i >= 11)
			entry.wchar3[i - 11] = value;
		else if (i >= 5)
			entry.wchar2[i - 5] = value;
		else
			entry.wchar1[i] = value;

		// Was this the last needed entry
		if (value == 0)
		{
			last = true;
			break;
		}
	}

	if (last)
		entry.index |= 0x40; // Last LFN entry

	return entry;
}

// Converts a filename to its 8.3 name
static char* toShortName(char *name)
{
	char *shortName = kmalloc(11);

	// Initialize the name with spaces
	for (int i = 0; i < 11; i++)
		shortName[i] = ' ';

	int len = strlen(name);
	int dotIndex = len; // No extension

	// May have extenstion
	if (len > 2)
	{
		for (int i = len - 1; i >= 0; i--)
		{
			if (name[i] == '.')
			{
				// Save the index of the last dot
				dotIndex = i;
				break;
			}
		}
	}

	int index = 0;

	// Copy the last 8 chars before the extension into the name
	for (int i = 0; i < dotIndex && index < 8; i++)
		shortName[index++] = toupper(name[i]);

	index = 8;

	// Copy the first 3 chars after the dot into the extension
	for (int i = dotIndex + 1; i < len && index < 11; i++)
		shortName[index++] = toupper(name[i]);

	return shortName;
}

// Get the full name for the file
static char *getFullName(dir_entry_t *entry, lfn_chain_t *lfn)
{
	char *fullname = NULL;

	// Parse name
	if (lfn)
		fullname = parseLongName(entry, lfn);
	
	if (!fullname) // May happen if lfn checksum check fails
		fullname = parseShortName(entry);

	return fullname;
}

// Adds the lfn entry into the lfn chain
static void addLFNEntry(lfn_chain_t **chain, lfn_entry_t *entry)
{
	lfn_chain_t *newEntry = kzalloc(sizeof(lfn_chain_t));
	newEntry->index = entry->index & 0b00011111;
	newEntry->checksum = entry->checksum;

	for (int i = 0; i < 13; i++)
	{
		uint16_t value = 0xFFFF;

		// Get correct value
		if (i >= 11)
			value = entry->wchar3[i - 11];
		else if (i >= 5)
			value = entry->wchar2[i - 5];
		else
			value = entry->wchar1[i];

		char chr;

		if (value == 0xFFFF)   // Unused char
			chr = 0;
		else if (value > 0xFF) // No support for UCS-2 charset
			chr = ' ';
		else                   // In ASCII range
			chr = (char)value;

		newEntry->part[i] = chr;
	}

	if (*chain == NULL)
		*chain = newEntry;
	else
	{
		lfn_chain_t *current = *chain;

		// Entry comes before the chain
		if (current->index > newEntry->index)
		{
			*chain = newEntry;
			newEntry->next = current;
			current->prev = newEntry;
		}
		else
		{
			for (lfn_chain_t *current = *chain; current; current = current->next)
			{
				if (current->index < newEntry->index)
				{
					// Insert between two entries
					if (current->next && current->next->index > newEntry->index)
					{
						current->next->prev = newEntry;
						newEntry->next = current->next;
						current->next = newEntry;
						newEntry->prev = current;
						break;
					}
					else if (!current->next) // Insert after chain
					{
						current->next = newEntry;
						newEntry->prev = current;
						break;
					}
				}
			}
		}
	}
}

// Parses the 8.3 filename of the directory entry
static char *parseShortName(dir_entry_t *entry)
{
	// Calculate the real size for the name and extension part
	// as they are padded with spaces

	int nameLength = 8;
	for (int i = 7; i >= 0; i--)
	{
		if (entry->name[i] != ' ')
			break;

		nameLength--;
	}

	int extLength = 3;
	for (int i = 2; i >= 0; i--)
	{
		if (entry->extension[i] != ' ')
			break;

		extLength--;
	}

	// 11 chars (+ dot) + null byte
	char *fullname = kmalloc(nameLength + extLength + 1 + (extLength > 0 ? 1 : 0));

	// Copy entry name to full name buffer
	for (int i = 0; i < nameLength; i++)
		fullname[i] = entry->name[i];

	if (extLength > 0) // Does the name have an extension?
	{
		fullname[nameLength] = '.';
		
		for (int i = 0; i < extLength; i++)
			fullname[nameLength + 1 + i] = entry->extension[i];

		fullname[nameLength + extLength + 1] = '\0';
	}
	else
		fullname[nameLength] = '\0';

	// Apply special filename types from Windows NT
	if (entry->nametype == LOWERCASE_EXT)
	{
		for (int i = 0; i < extLength; i++)
			fullname[nameLength + 1 + i] = tolower(fullname[9 + i]);
	}
	else if (entry->nametype == LOWERCASE_BASE)
	{
		for (int i = 0; i < nameLength; i++)
			fullname[i] = tolower(fullname[i]);
	}

	return fullname;
}

// Parses the lfn chain and allocates a string for it
static char *parseLongName(dir_entry_t *entry, lfn_chain_t *lfn)
{
	int length = 0;
	uint8_t checksum = calcChecksum(entry->name);

	// Count entries
	for (lfn_chain_t *current = lfn; current; current = current->next)
	{
		// The long file name doesn't belong to the 8.3 entry
		if (checksum != current->checksum)
			return NULL;

		if (current->next)
			length += 13;
		else
		{
			for (int i = 0; i < 13; i++)
			{
				if (current->part[i] == 0)
					break;
				length++;
			}

			length++; // Trailing null terminator
		}
	}

	char *fullname = kmalloc(length);

	// Copy the substrings into the name buffer at their respective indices
	lfn_chain_t *current = lfn;
	for (int i = 0; i < length - 1; i++)
	{
		if (i > 0 && i % 13 == 0)
			current = current->next;

		fullname[i] = current->part[i % 13];
	}

	fullname[length - 1] = 0;

	return fullname;
}

// Initializes the . and .. entries for a directory
// The directory is assumed to be empty
static int initDirectory(file_desc_t *dir)
{
	fat32_metadata_t *data = (fat32_metadata_t*)dir->mount->metadata;

	file_desc_t tmp = { 0 }; // Initialize with zeroes
	tmp.flags = FS_DIRECTORY;
	
	tmp.name[0] = '.';      // Construct . filename
	tmp.inode = dir->inode; // Point to current dir
	dir_entry_t dotEntry = toDirEntry(&tmp);

	tmp.name[1] = '.';              // Construct .. filename
	tmp.inode = dir->parent->inode; // Point to parent dir
	dir_entry_t dotdotEntry = toDirEntry(&tmp);

	uint8_t *buf = kzalloc(data->bytesPerCluster);
	*(dir_entry_t*)&buf[0] = dotEntry;
	*(dir_entry_t*)&buf[sizeof(dir_entry_t)] = dotdotEntry;

	// Update directory (assumed to be empty eg. only one empty cluster)
	cluster_chain_t *chain = getChain(dir->mount, dir->inode);
	int ret = doClusterOperation(buf, dir->mount, chain, 0, WRITE);
	deleteClusterChain(chain);
	kfree(buf);

	return ret;
}

// Creates a file descriptor describing the file declared in the directory entry
static file_desc_t *createFile(file_desc_t *root, dir_chain_t *direntry)
{
	file_desc_t *file = kzalloc(sizeof(file_desc_t));

	if (direntry->entry.attributes & DIRECTORY)
	{
		// Write directory callbacks
		file->flags = FS_DIRECTORY;
		file->findfile = (findfile_callback)findfileFAT32;
		file->mkfile = (mkfile_callback)mkfileFAT32;
		file->rmfile = (rmfile_callback)rmfileFAT32;
		file->readdir = (readdir_callback)readdirFAT32;
		file->length = 0;
	}
	else
	{
		// Write file callbacks
		file->flags = FS_FILE;
		file->read = (read_callback)readFAT32;
		file->write = (write_callback)writeFAT32;
		file->length = direntry->entry.size;
	}

	file->rename = (rename_callback)renameFAT32;

	// Write remaining metadata
	file->parent = root;
	file->mount = root->mount;
	file->inode = cluster(&direntry->entry);
	strcpy(file->name, direntry->fullname);

	return file;
}

// Writes/reads a specific part from/into the buffer into/from the file
static size_t doFileOperation(file_desc_t *file, size_t offset, size_t size, char *buf, bool write)
{
	if (size == 0 || offset > file->length)
		return 0;

	cluster_chain_t *chain = getChain(file->mount, file->inode);
	fat32_metadata_t *metadata = (fat32_metadata_t*)file->mount->metadata;
	uint32_t bpc = metadata->bytesPerCluster; // Abbreviation as the value gets used often

	// Operation will go out of bounds?
	if (size > file->length - offset)
	{
		// Try to extend the file
		if (write)
		{
			// Round up new size to full clusters
			int toAllocate = (((offset + size) + bpc - 1) / bpc) - getClusterCount(chain);

			// Allocated necessary clusters
			for (; toAllocate > 0; toAllocate--)
			{
				if (!addCluster(chain, file->mount))
				{
					debug_set_color(0x0C, 0x00);
					debug_print("The partition is full! Could'nt allocate a new cluster!");
					debug_set_color(0x0F, 0x00);

					kfree(chain);
					return 0;
				}
			}

			// Update file length and directory entry
			file->length = offset + size;
		}
		else // Limit read amount
			size = file->length - offset;
	}

	// Calculate cluster area to read from
	uint32_t first = offset / bpc;
	uint32_t count = ((offset + size) + bpc - 1) / bpc - first;
	uint32_t last = first + (count - 1);

	if (first >= getClusterCount(chain))
	{
		kfree(chain);
		return 0;
	}

	if (last >= getClusterCount(chain))
		last = getClusterCount(chain) - 1;

	size_t index = 0;
	size_t start, end, amount;

	char *tmpBuf = kmalloc(metadata->bytesPerCluster);
	
	for (uint32_t i = first; i <= last; i++)
	{
		// Calculate start and end offsets
		start = i == first ? (offset % bpc) : 0;
		end = i == last ? ((offset + size) % bpc) : bpc;
		if (end == 0)
			end = bpc;

		amount = end - start; // Bytes to read/write

		if (write)
		{
			//If we don't write from start to end of the cluster get the content of the cluster
			if(start != 0 || end != bpc)
			{
				if (doClusterOperation(tmpBuf, file->mount, chain, i, false))
				{
					debug_set_color(0x0C, 0x00);
					debug_print("Failed to operate on cluster");
					debug_set_color(0x0F, 0x00);

					deleteClusterChain(chain);
					kfree(tmpBuf);
					return index;
				}
			}
			memcpy(tmpBuf + start, buf + index, amount);
		}
		
		if (doClusterOperation(tmpBuf, file->mount, chain, i, write))
		{
			debug_set_color(0x0C, 0x00);
			debug_print("Failed to operate on cluster");
			debug_set_color(0x0F, 0x00);

			deleteClusterChain(chain);
			kfree(tmpBuf);
			return index;
		}
		
		if (!write)
			memcpy(buf + index, tmpBuf + start, amount);

		index += amount;
	}

	kfree(tmpBuf);

	if (write) // Write may change file length data
		updateDirectoryEntry(file->parent, file, file->name);

	deleteClusterChain(chain);

	return index;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

size_t readFAT32(file_desc_t *node, size_t offset, size_t size, char *buf)
{
	return doFileOperation(node, offset, size, buf, READ);
}

size_t writeFAT32(file_desc_t *node, size_t offset, size_t size, char *buf)
{
	return doFileOperation(node, offset, size, buf, WRITE);
}

int readdirFAT32(DIR *dirstream)
{
	dir_chain_t *directory = parseDirectory(dirstream->dirfile);

	// Get correct directory entry
	dir_chain_t *current = directory;
	for (size_t i = 0; i < dirstream->index && current; i++, current = current->next);

	int ret = 0;

	// Is there a next entry?
	if (current)
	{
		// Is the name small enough?
		if (strlen(current->fullname) > FILENAME_MAX)
			ret = DIROVERFLOW;
		else
		{
			// Copy the name into the dirent
			strcpy(dirstream->entry.d_name, current->fullname);
			dirstream->entry.d_ino = cluster(&current->entry);

			// Determine file type
			if (current->entry.attributes & DIRECTORY)
				dirstream->entry.d_type = DT_DIR;
			else
				dirstream->entry.d_type = DT_REG;
		}
	}
	else
		ret = EOF;

	deleteDirectoryChain(directory);

	return ret;
}

file_desc_t *findfileFAT32(file_desc_t *node, char *name)
{
	debug_printf("findfileFAT32: %s", name);

	dir_chain_t *directory = parseDirectory(node);

	for (dir_chain_t *current = directory; current; current = current->next)
	{
		if (strcmp(current->fullname, name) == 0)
		{
			file_desc_t *file = createFile(node, current);
			deleteDirectoryChain(directory);
			return file;
		}
	}

	deleteDirectoryChain(directory);

	return 0;
}

int mkfileFAT32(file_desc_t *file)
{
	// File needs a cluster chain?
	if (file->inode == 0)
	{
		cluster_chain_t *fileChain = createChain(file->mount, 1);

		if (!fileChain)
			return EOF;

		file->length = 0;
		file->inode = fileChain->index;

		deleteClusterChain(fileChain);
	}

	if(addDirectoryEntry(file->parent, file))
		return EOF;

	// Save callbacks
	if (file->flags & FS_FILE)
	{
		file->read = (read_callback)readFAT32;
		file->write = (write_callback)writeFAT32;
	}
	else
	{
		file->findfile = (findfile_callback)findfileFAT32;
		file->mkfile = (mkfile_callback)mkfileFAT32;
		file->readdir = (readdir_callback)readdirFAT32;
		file->rmfile = (rmfile_callback)rmfileFAT32;

		if (initDirectory(file))
			return EOF;
	}

	file->rename = (rename_callback)renameFAT32;

	return 0;
}

int rmfileFAT32(file_desc_t *file)
{
	cluster_chain_t *chain = getChain(file->mount, file->inode);

	int ret = 0;

	if (shrinkChain(file->mount, chain, 0))
		ret = EOF;

	if (ret == 0 && removeDirectoryEntry(file->parent, file->name, file->inode))
		ret = EOF;
	
	deleteClusterChain(chain);

	return ret;
}

int renameFAT32(file_desc_t *file, file_desc_t *newParent, char *origName)
{
	// Remove entry from original directory
	if (removeDirectoryEntry(file->parent, origName, file->inode))
		return EOF;

	// Add entry to new directory
	if (addDirectoryEntry(newParent, file))
	{
		// Add it back to the original directory if the change fails
		addDirectoryEntry(file->parent, file);
		return EOF;
	}

	return 0;
}

// Mount the filesystem by reading the BPB and FSInfo structs and saving them as metadata
mountpoint_t *mountFAT32(partition_t *partition)
{
	debug_printf("mountFAT32");

	// Read in BPB block
	bpb_t *bpb = kmalloc(sizeof(bpb_t));
	if (ataRead((void*)bpb, partition->offset, 1, partition->device))
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Couldn't read BPB");
		debug_set_color(0x0F, 0x00);
		kfree(bpb);
		return NULL;
	}

	// Read in FSInfo struct
	fsinfo_t *fsinfo = kmalloc(sizeof(fsinfo_t));
	if (ataRead((void*)fsinfo, partition->offset + bpb->ebpb.fsinfoSector, 1, partition->device))
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Couldn't read FSInfo struct");
		debug_set_color(0x0F, 0x00);
		kfree(bpb);
		kfree(fsinfo);
		return NULL;
	}

	mountpoint_t *mount = kmalloc(sizeof(mountpoint_t));

	// Create mountpoint struct
	fat32_metadata_t *metadata = kmalloc(sizeof(fat32_metadata_t));
	metadata->bpb = bpb;
	metadata->fsinfo = fsinfo;
	metadata->bytesPerCluster = bpb->bytesPerSector * bpb->sectorsPerCluster;
	metadata->firstFATSector = /*partition->offset + */ bpb->hiddenSectors + bpb->reservedSectors;
	metadata->firstDataSector = metadata->firstFATSector + (bpb->numberOfFATs * bpb->ebpb.sectorsPerFAT);

	// Create root file descriptor
	file_desc_t *rootdir = kzalloc(sizeof(file_desc_t));
	rootdir->flags = FS_DIRECTORY;
	rootdir->length = 0;
	rootdir->parent = NULL;
	rootdir->mount = mount; // Save mount metadata
	rootdir->inode = bpb->ebpb.rootcluster;
	rootdir->findfile = (findfile_callback)findfileFAT32;
	rootdir->readdir = (readdir_callback)readdirFAT32;
	rootdir->mkfile = (mkfile_callback)mkfileFAT32;
	rootdir->rmfile = (rmfile_callback)rmfileFAT32;

	debug_printf("rootdir->findfile: %p", rootdir->findfile);

	// Create the mountpoint
	mount->partition = partition;
	mount->root = rootdir;
	mount->metadata = (uintptr_t)metadata;
	mount->unmount = (unmount_callback)unmountFAT32;

	return mount;
}

// Unmount the filesystem by freeing the used memory
void unmountFAT32(mountpoint_t *mountpoint)
{
	// Free used memory
	fat32_metadata_t *metadata = (fat32_metadata_t*)mountpoint->metadata;
	kfree(metadata->bpb);
	kfree(metadata->fsinfo);
	kfree(metadata);
}
