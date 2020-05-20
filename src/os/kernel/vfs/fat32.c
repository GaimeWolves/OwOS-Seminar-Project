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

static uint8_t calcChecksum(const dir_entry_t *entry);

static void deleteClusterChain(cluster_chain_t *chain);
static void deleteDirectoryChain(dir_chain_t *chain);
static void deleteLFNChain(lfn_chain_t *chain);

static uint32_t getClusterSector(mountpoint_t *metadata, uint32_t cluster);
static cluster_chain_t *getChain(mountpoint_t *metadata, uint32_t first);
static size_t getClusterCount(cluster_chain_t *chain);
static int readCluster(void* buf, mountpoint_t *metadata, cluster_chain_t *chain, uint32_t offset);

static dir_chain_t *parseDirectory(file_desc_t *file);
static char *getFullName(dir_entry_t *entry, lfn_chain_t *lfn);
static void addLFNEntry(lfn_chain_t **chain, lfn_entry_t *entry);
static char *parseShortName(dir_entry_t *entry);
static char *parseLongName(dir_entry_t *entry, lfn_chain_t *lfn);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

static inline uint32_t cluster(dir_entry_t *entry)
{
	return (uint32_t)entry->clusterLow | ((uint32_t)entry->clusterHigh << 16);
}

// Calculates the cecksum present on long file name entries
// from the 8.3 filename
static uint8_t calcChecksum(const dir_entry_t *entry)
{
	uint8_t *filename = (uint8_t*)&entry->name;

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

	cluster_chain_t *chain = kmalloc(sizeof(cluster_chain_t));
	chain->index = first;

	cluster_chain_t *current = chain;

	while (true)
	{
		uint8_t table[data->bpb->bytesPerSector];                                      // Subset of FAT table
		uint32_t offset = current->index * 4;                                          // Byte offset
		uint32_t sector = data->firstFATSector + (offset / data->bpb->bytesPerSector); // Sector to read
		uint32_t sectorOffset = offset % data->bpb->bytesPerSector;                    // Sector offset

		if (ataRead((void*)table, sector, 1, metadata->partition->device))
		{
			debug_set_color(0x0C, 0x00);
			debug_printf("Couldn't read cluster value %u", current->index);
			debug_set_color(0x0F, 0x00);
			return NULL;
		}

		// Read the cluster value and mask upper 4 bits
		uint32_t value = *(uint32_t*)&table[sectorOffset] & 0x0FFFFFFF;
		current->value = value;

		if (value > 0x1 && value < 0x0FFFFFF7)
		{
			// Add new cluster to chain
			cluster_chain_t *next = kmalloc(sizeof(cluster_chain_t));
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

// Get the amount of clusters of the chain
static size_t getClusterCount(cluster_chain_t *chain)
{
	size_t len = 0;

	cluster_chain_t *current = chain;

	while (current)
	{
		len++;
		current = current->next;
	}

	return len;
}

static int readCluster(void* buf, mountpoint_t *metadata, cluster_chain_t *chain, uint32_t offset)
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
	if (ataRead(buf, getClusterSector(metadata, current->index), data->bpb->sectorsPerCluster, metadata->partition->device))
	{
		debug_set_color(0x0C, 0x00);
		debug_printf("Couldn't read cluster %u", current->index);
		debug_set_color(0x0F, 0x00);
		return -2;
	}

	return 0;
}

// Reads the complete directory into a traversable chain
static dir_chain_t *parseDirectory(file_desc_t *file)
{
	cluster_chain_t *chain = getChain(file->mount, file->inode);

	fat32_metadata_t *metadata = (fat32_metadata_t*)file->mount->metadata;

	uint8_t* buf = kmalloc(metadata->bytesPerCluster);
	uint32_t offset = 0;
	int cluster = 0;
	
	if (readCluster(buf, file->mount, chain, 0))
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Directory is corrupted or couldn't be read");
		debug_set_color(0x0F, 0x00);
		return NULL;
	}

	lfn_chain_t *lfn = NULL;
	dir_chain_t *dir = NULL;
	dir_chain_t *current = NULL;

	// Read all directory entries
	while(true)
	{
		// Read next cluster into buffer
		if (offset >= metadata->bytesPerCluster)
		{
			offset = 0;
			cluster++;
			
			int ret = readCluster(buf, file->mount, chain, cluster);
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
		else if (buf[offset] == 0xE5) // Unused entry
		{
			offset += sizeof(dir_entry_t);
			continue;
		}
		else if (buf[offset + 11] == 0x0F) // Long file name entrie
		{
			lfn_entry_t *lfnEntry = (lfn_entry_t*)&buf[offset];
			addLFNEntry(&lfn, lfnEntry);
		}
		else // Normal directory entry
		{
			dir_entry_t *entry = (dir_entry_t*)&buf[offset];
			char *fullname = getFullName(entry, lfn);
			deleteLFNChain(lfn);
			lfn = NULL;

			// Create new entry
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

		offset += sizeof(dir_entry_t);
	}

	deleteLFNChain(lfn);
	deleteClusterChain(chain);
	kfree(buf);

	return dir;
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

		// Little-Endian
		// value = (value >> 8) | (value << 8);

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
	// 11 chars + dot + null byte
	char *fullname = kmalloc(13);

	// Copy entry name to full name buffer
	for (int i = 0; i < 8; i++)
		fullname[i] = entry->name[i];

	fullname[8] = '.';

	for (int i = 0; i < 3; i++)
		fullname[9 + i] = entry->extension[i];

	fullname[12] = 0;

	// Apply special filename types from Windows NT
	if (entry->nametype == LOWERCASE_EXT)
	{
		for (int i = 0; i < 3; i++)
			fullname[9 + i] = tolower(fullname[9 + i]);
	}
	else if (entry->nametype == LOWERCASE_BASE)
	{
		for (int i = 0; i < 8; i++)
			fullname[i] = tolower(fullname[i]);
	}

	return fullname;
}

// Parses the lfn chain and allocates a string for it
static char *parseLongName(dir_entry_t *entry, lfn_chain_t *lfn)
{
	int lenght = 0;
	uint8_t checksum = calcChecksum(entry);

	// Count entries
	for (lfn_chain_t *current = lfn; current; current = current->next)
	{
		// The long file name doesn't belong to the 8.3 entry
		if (checksum != current->checksum)
			return NULL;

		if (current->next)
			lenght += 13;
		else
		{
			for (int i = 0; i < 13; i++)
			{
				if (current->part[i] == 0)
					break;
				lenght++;
			}

			lenght++; // Trailing null terminator
		}
	}

	char *fullname = kmalloc(lenght);

	lfn_chain_t *current = lfn;
	for (int i = 0; i < lenght - 1; i++)
	{
		if (i > 0 && i % 13 == 0)
			current = current->next;

		fullname[i] = current->part[i % 13];
	}

	fullname[lenght - 1] = 0;

	return fullname;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

int readFAT32(file_desc_t *node, size_t offset, int size, char *buf)
{
	return 0;
}

int writeFAT32(file_desc_t *node, size_t offset, int size, char *buf)
{
	return 0;
}

int readdirFAT32(DIR *dirstream)
{
	dir_chain_t *directory = parseDirectory(dirstream->dirfile);

	// Get correct directory entry
	dir_chain_t *current = directory;
	for (size_t i = 0; i < dirstream->index && current; i++, current = current->next);

	int ret;

	if (current)
	{
		if (strlen(current->fullname) > FILENAME_MAX)
			ret = EOVERFLOW;
		else
		{
			strcpy(dirstream->entry.name, current->fullname);
			dirstream->entry.inode = cluster(&current->entry);
		}

		ret = 0;
	}
	else
		ret = EOF;

	deleteDirectoryChain(directory);

	return ret;
}

file_desc_t *findfileFAT32(file_desc_t *node, char *name)
{
	return 0;
}

file_desc_t *mkdirFAT32(file_desc_t *node, char *name)
{
	return 0;
}

file_desc_t *mkfileFAT32(file_desc_t *node, char *name)
{
	return 0;
}

// Mount the filesystem by reading the BPB and FSInfo structs and saving them as metadata
mountpoint_t *mountFAT32(partition_t *partition)
{
	// Read in BPB block
	bpb_t *bpb = kmalloc(sizeof(bpb_t));
	if (ataRead((void*)bpb, partition->offset, 1, partition->device))
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Couldn't read BPB");
		debug_set_color(0x0F, 0x00);
		return NULL;
	}

	// Read in FSInfo struct
	fsinfo_t *fsinfo = kmalloc(sizeof(fsinfo_t));
	if (ataRead((void*)fsinfo, partition->offset + bpb->ebpb.fsinfoSector, 1, partition->device))
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Couldn't read FSInfo struct");
		debug_set_color(0x0F, 0x00);
		return NULL;
	}

	mountpoint_t *mount = kmalloc(sizeof(mountpoint_t));

	// Create mountpoint struct
	fat32_metadata_t *metadata = kmalloc(sizeof(fat32_metadata_t));
	metadata->bpb = bpb;
	metadata->fsinfo = fsinfo;
	metadata->bytesPerCluster = bpb->bytesPerSector * bpb->sectorsPerCluster;
	metadata->firstFATSector = partition->offset + bpb->hiddenSectors + bpb->reservedSectors;
	metadata->firstDataSector = metadata->firstFATSector + (bpb->numberOfFATs * bpb->ebpb.sectorsPerFAT);

	// Create root file descriptor
	file_desc_t *rootdir = kzalloc(sizeof(file_desc_t));
	rootdir->flags = FS_DIRECTORY;
	rootdir->mount = mount; // Save mount metadata
	rootdir->inode = bpb->ebpb.rootcluster;
	rootdir->findfile = (findfile_callback)findfileFAT32;
	rootdir->readdir = (readdir_callback)readdirFAT32;
	rootdir->mkdir = (mkdir_callback)mkdirFAT32;
	rootdir->mkfile = (mkfile_callback)mkfileFAT32;

	// Create the mountpoint
	mount->partition = partition;
	mount->root = rootdir;
	mount->metadata = (uintptr_t)metadata;
	mount->unmount = (unmount_callback)unmountFAT32;

	// Get size of root directory
	cluster_chain_t *rootchain = getChain(mount, rootdir->inode);
	rootdir->length = getClusterCount(rootchain) * bpb->bytesPerSector;
	deleteClusterChain(rootchain);

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
