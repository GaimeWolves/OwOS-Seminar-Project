#include <vfs/vfs.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <memory/heap.h>
#include <hal/atapio.h>
#include <vfs/mbr.h>
#include <vfs/pathutils.h>
#include <string.h>
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
	struct file_desc_t *file_desc;

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

static file_desc_t *findfile(vfs_node_t *node, char *path);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

static file_desc_t *findfile(vfs_node_t *node, char *path)
{
	// Does the node refer to another node? (eg. symlinks or mountpoints)
	if (node->file_desc->flags & (FS_MOUNTPOINT | FS_SYMLINK))
		node = node->child;
	
	// Correct node found
	if (getPathLength(path) == 0)
		return node->file_desc;

	// Is the (new) node a directory?
	if (!(node->file_desc->flags & FS_DIRECTORY))
		return NULL;

	// Get file string
	char *file = getPathSubstr(path, 0);
	rmPathDirectory(path);

	// Check if node is already in memory (recursively traverse node tree)
	for (vfs_node_t *child = node->child; child; child = child->next)
	{
		if (strcmp(child->file_desc->name, file) == 0)
		{
			kfree(file);
			return findfile(child, path);
		}
	}

	// Get file via filesystem driver
	file_desc_t *newFile = node->file_desc->findfile(node->file_desc, file);

	if (!newFile)
	{
		kfree(file);
		return NULL;
	}

	vfs_node_t *newNode = kzalloc(sizeof(vfs_node_t));
	newNode->file_desc = newFile;

	// Insert node
	node->child->prev = newNode;
	newNode->next = node->child;
	node->child = newNode;

	kfree(file);
	return findfile(newNode, path);
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Tries to identify the root partition the kernel resides in
// and mounts it as the root node (path: / )
int initVFS()
{
	// Find all devices using MBR partitioning
	initMBR();

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

	mountpoint_t *mount = mountFAT32(bootpart);

	if (!mount)
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
	root->file_desc = mount->root;

	return 0;
}

FILE* vfsOpen(const char *path, int flags);
void vfsClose(FILE *file);
size_t vfsRead(FILE *file, void *buf, size_t size);
size_t vfsWrite(FILE *file, const void *buf, size_t size);

int vfsSeek(FILE *file, size_t offset, int origin);
int vfsFlush(FILE *file);
int vfsSetvbuf(FILE *file, char *buf, int mode, size_t size);
int vfsRename(char *oldPath, char *newPath);
int vfsRemove(char *path);
int vfsMkdir(char *path);

DIR *vfsOpendir(char *path)
{
	rmPathDirectory(path);

	DIR *dir = kzalloc(sizeof(DIR));
	dir->dirfile = findfile(root, path);

	if (!dir->dirfile)
	{
		kfree(dir);
		return NULL;
	}

	if (!(dir->dirfile->flags & FS_DIRECTORY))
	{
		kfree(dir);
		return NULL;
	}

	dir->dirfile->openReadDesc++;

	return dir;
}

int vfsCloseDir(DIR *dir)
{
	dir->dirfile->openReadDesc--;
	kfree(dir);

	return 0;
}

dirent *vfsReaddir(DIR *dir)
{
	int ret = dir->dirfile->readdir(dir);

	if (ret != EOF)
		dir->index++;

	if (ret)
		return NULL;

	return &dir->entry;
}
