#include <vfs/vfs.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <memory/heap.h>
#include <hal/atapio.h>
#include <vfs/mbr.h>
#include <debug.h>

#include <vfs/fat32.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

// Datastructure to represent the node graph of the filesystem currently in memory
typedef struct vfs_node_t
{
	file_desc_t *file_desc;

	// Pointer to first entry inside directory
	// (applies if associated file is a directory or mountpoint)
	struct vfs_node_t *child;

	// Pointers to next and previous directory entries
	// (applies if there are more files inside the directory this file resides in)
	struct vfs_node_t *prev;
	struct vfs_node_t *next;
} vfs_node_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

// Root node of the vfs node graph
static vfs_node_t *root;

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Tries to identify the root partition the kernel resides in
// and mounts it as the root node (path: / )
int initVFS()
{
	partition_t *bootpart = NULL;

	// Iterate through ATA devices (the only supported type)
	for (int device = 0; device < ATA_MAX_DRIVES; device++)
	{
		disk_t *disk = getPartitionInfo(device);

		// Iterate through partitions
		for (int partIndex = 0; partIndex < MBR_MAX_PARTITIONS; partIndex++)
		{
			partition_t *partition = &disk->partitions[partIndex];

			// Is the partition the boot partition?
			if (partition->active)
			{
				bootpart = partition;
				break;
			}
		}

		// We found the root partition
		if (bootpart)
			break;
	}

	if (!bootpart)
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Couldn't find root partition! Cannot mount VFS root!");
		debug_set_color(0x0F, 0x00);
		return -1;
	}

	// Do we have a filesystem driver for the root partition?
	if (bootpart->type != FAT32_LBA)
	{
		debug_set_color(0x0C, 0x00);
		debug_print("The root partitions filesystem type is unsupported!");
		debug_set_color(0x0F, 0x00);
		return -1;
	}

	file_desc_t *rootfile = mountFAT32(bootpart);

	if (!rootfile)
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Could not mount the root filesystem!");
		debug_set_color(0x0F, 0x00);
		return -1;
	}

	// Allocate VFS root on kernel heap
	root = kzalloc(sizeof(vfs_node_t));

	if (!root)
	{
		debug_set_color(0x0C, 0x00);
		debug_print("Could not allocate root vfs node");
		debug_set_color(0x0F, 0x00);
		return -1;
	}

	// Save the root filesystem inside the root vfs node
	root->file_desc = rootfile;

	return 0;
}
