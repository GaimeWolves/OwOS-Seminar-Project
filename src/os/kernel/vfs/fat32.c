#include <vfs/fat32.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <memory/heap.h>
#include <hal/atapio.h>
#include <vfs/vfs.h>
#include <debug.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define DIR_ENTRY_SIZE 32

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

// Mountpoint structure for FAT32
typedef struct fat32_mountpoint_t
{
	partition_t *partition;
	bpb_t *bpb;
	fsinfo_t *fsinfo;
} fat32_mountpoint_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

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

dirent *readdirFAT32(file_desc_t *node, int index)
{
	return 0;
}

file_desc_t *findfileFAT32(file_desc_t *node, char *name)
{
	return 0;
}

file_desc_t *mountFAT32(partition_t *partition)
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

	// Create mountpoint struct
	fat32_mountpoint_t *mount = kmalloc(sizeof(fat32_mountpoint_t));
	mount->partition = partition;
	mount->bpb = bpb;
	mount->fsinfo = fsinfo;

	// Create file descriptor
	file_desc_t *rootdir = kzalloc(sizeof(file_desc_t));
	rootdir->flags = FS_DIRECTORY;
	rootdir->impl = (uintptr_t)mount; // Save mount info
	rootdir->inode = bpb->ebpb.rootcluster;
	rootdir->length = bpb->directoryEntries * DIR_ENTRY_SIZE;
	rootdir->findfile = (findfile_callback)findfileFAT32;
	rootdir->readdir = (readdir_callback)readdirFAT32;
	rootdir->mkdir = (mkdir_callback)mkdirFAT32;
	rootdir->mkfile = (mkfile_callback)mkfileFAT32;

	return rootdir;
}

file_desc_t *mkdirFAT32(file_desc_t *node, char *name)
{
	return 0;
}

file_desc_t *mkfileFAT32(file_desc_t *node, char *name)
{
	return 0;
}
