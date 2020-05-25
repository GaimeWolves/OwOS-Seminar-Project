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

// Private file flags
#define ORIGBUF  0x40000000 // Original buffer still present (gets cleared on vfsSetvbuf)

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

// Datastructure to represent the node graph of the filesystem currently in memory
typedef struct vfs_node_t
{
	struct file_desc_t *file_desc;

	// Pointer to the directory this node lies in
	struct vfs_node_t *parent;

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

static vfs_node_t *findfile(vfs_node_t *node, char *path);
static vfs_node_t *createFile(vfs_node_t *node, char *path, uint32_t flags);
static int removeNode(vfs_node_t *node);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

static vfs_node_t *findfile(vfs_node_t *node, char *path)
{
	// Does the node refer to another node? (eg. symlinks or mountpoints)
	if (node->file_desc->flags & (FS_MOUNTPOINT | FS_SYMLINK))
		node = node->child;
	
	// Correct node found
	if (getPathLength(path) == 0)
		return node;

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
	newNode->parent = node;
	node->child->prev = newNode;
	newNode->next = node->child;
	node->child = newNode;

	kfree(file);
	return findfile(newNode, path);
}

static vfs_node_t *createFile(vfs_node_t *node, char *path, uint32_t flags)
{
	// File already exists
	if (findfile(node, path))
		return NULL;

	char *dirPath = getPathDir(path);
	char *filename = getPathFile(path);

	if (!filename)
	{
		if (dirPath) kfree(dirPath);
		return NULL;
	}

	vfs_node_t *parent = NULL;
	if (!dirPath)
		parent = node;
	else
		parent = findfile(node, dirPath);
	
	if (dirPath) kfree(dirPath);

	if (!parent)
	{
		kfree(filename);
		return NULL;
	}

	file_desc_t *file = kzalloc(sizeof(file_desc_t));
	file->flags = flags;
	file->mount = node->file_desc->mount;

	strcpy(file->name, filename);

	if (parent->file_desc->mkfile(file))
	{
		kfree(filename);
		kfree(file);
		return NULL;
	}

	vfs_node_t *newNode = kzalloc(sizeof(vfs_node_t));
	newNode->file_desc = file;
	newNode->parent = node;

	if (parent->child)
	{
		parent->child->prev = newNode;
		newNode->next = parent->child;
	}

	parent->child = newNode;

	return newNode;
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

FILE* vfsOpen(const char *path, const char *mode)
{
	char *copy = kstrdup(path);
	rmPathDirectory(copy);

	vfs_node_t *node = findfile(root, copy);

	strcpy(copy, path);
	rmPathDirectory(copy);

	// Read mode requires an existing file
	if (!node && mode[0] == 'r')
	{
		kfree(copy);
		return NULL;
	}
	else if (!node)
		node = createFile(root, copy, FS_FILE);

	kfree(copy);

	if (node)
		return NULL;

	// Cannot open a file in write mode more than once
	if (node->file_desc->openWriteDesc > 0 && (mode[0] == 'w' || mode[0] == 'a' || mode[1] == '+'))
		return NULL;

	// Allocate file
	FILE *file = kzalloc(sizeof(FILE));
	file->file_desc = node->file_desc;

	// Parse mode
	size_t len = strlen(mode);
	switch(mode[0])
	{
		case 'r':
			file->flags |= O_RDONLY;
			break;
		case 'w':
			file->flags |= O_WRONLY | O_TRUNC;
			break;
		case 'a':
			file->flags |= O_WRONLY | O_APPEND;
			break;
	}

	if (len == 3) // Both flags present (eg. w+b, wb+, a+b, etc.)
	{
		file->flags |= O_BIN;
		file->flags |= O_RDWR;
	}
	else if (len == 2) // Only one flag present (eg. wb or w+)
	{
		if (mode[1] == '+')
			file->flags |= O_RDWR;
		else
			file->flags |= O_BIN;
	}

	// Update read/write counts
	if (file->flags & O_RDWR)
	{
		node->file_desc->openReadDesc++;
		node->file_desc->openWriteDesc++;
	}
	else if (file->flags & O_RDONLY)
		node->file_desc->openReadDesc++;
	else if (file->flags & O_WRONLY)
		node->file_desc->openWriteDesc++;

	// Initialize buffers
	char *ioBuf = kmalloc(BUFSIZ);
	file->ioBuf = ioBuf;
	file->ioEnd = file->ioBuf + BUFSIZ;
	file->rdBuf = file->ioBuf;
	file->rdPtr = file->rdBuf;
	file->rdFil = file->rdBuf;
	file->rdEnd = file->rdBuf + BUFSIZ / 2;
	file->wrBuf = file->ioBuf + BUFSIZ / 2;
	file->wrPtr = file->wrBuf;
	file->wrEnd = file->ioEnd;
	file->flags |= ORIGBUF;

	// Go to end in append mode
	if (file->flags & O_APPEND)
		vfsSeek(file, 0, SEEK_END);

	return file;
}

void vfsClose(FILE *file)
{
	if (!file)
		return;

	vfsFlush(file);

	// Update read/write counts
	if (file->flags & O_RDWR)
	{
		file->file_desc->openReadDesc--;
		file->file_desc->openWriteDesc--;
	}
	else if (file->flags & O_RDONLY)
		file->file_desc->openReadDesc--;
	else if (file->flags & O_WRONLY)
		file->file_desc->openWriteDesc--;

	if (file->flags & ORIGBUF)
		kfree(file->ioBuf);

	// TODO: Cleanup node tree
	
	kfree(file);
	return;
}

size_t vfsRead(FILE *file, void *buf, size_t size)
{
	if (!(file->flags & O_RDONLY || file->flags & O_RDWR))
		return 0;

	char *buffer = (char*)buf;

	size_t read = 0;
	size_t rdSize = (size_t)(file->rdEnd - file->rdBuf);

	// Read until EOF is reached or specified amount is read
	while(!(file->flags & O_EOF && file->rdPtr == file->rdFil) && read < size)
	{
		for (; file->rdPtr < file->rdFil && read < size; file->rdPtr++, read++)
			buffer[read] = *file->rdPtr;

		if (file->flags & O_EOF || read >= size)
			break;

		// Read next portion into buffer
		file->rdPtr = file->rdBuf;
		file->rdFil = file->rdBuf;

		size_t amount = file->file_desc->read(file->file_desc, file->pos, rdSize, file->rdBuf);
		file->pos += amount;
		file->rdFil += amount;

		// EOF reached
		if (amount < rdSize)
			file->flags |= O_EOF;
	}

	return read;
}

size_t vfsWrite(FILE *file, const void *buf, size_t size)
{
	if (!(file->flags & O_WRONLY || file->flags & O_RDWR))
		return 0;

	char *buffer = (char*)buf;

	size_t written = 0;
	size_t wrSize = (size_t)(file->wrEnd - file->wrBuf);

	// Write until buffer is empty
	while(written < size)
	{
		for (; file->wrPtr < file->wrEnd && written < size; file->wrPtr++, written++)
			*file->wrPtr = buffer[written];

		if (written >= size)
			break;

		size_t amount = file->file_desc->write(file->file_desc, file->pos, wrSize, file->wrBuf);
		file->pos += amount;
		file->wrPtr = file->wrBuf + (wrSize - amount);

		// Move unwritten contents to beginning
		memcpy(file->wrBuf, file->wrBuf + amount, wrSize - amount);

		if (amount < wrSize)
			break;
	}

	return written;
}

int vfsSeek(FILE *file, long offset, int origin)
{
	size_t pos = 0;

	// First flush unwritten data
	vfsFlush(file);
	
	switch(origin)
	{
		case SEEK_CUR:
			pos = file->pos;
			break;
		case SEEK_END:
			pos = file->file_desc->length;
			break;
		default:
			break;
	}

	// Check if seek results in underflow
	if (offset < 0 && pos < (size_t)(-offset))
		pos = -offset; // To end up with zero afterwards

	pos += offset;

	file->pos = pos;

	return 0;
}

int vfsFlush(FILE *file)
{
	if (!(file->flags & O_WRONLY || file->flags & O_RDWR))
		return EOF;

	size_t wrSize = (size_t)(file->wrPtr - file->wrBuf);

	size_t amount = file->file_desc->write(file->file_desc, file->pos, wrSize, file->wrBuf);
	file->pos += amount;
	file->wrPtr = file->wrBuf + (wrSize - amount);

	// Move unwritten contents to beginning
	memcpy(file->wrBuf, file->wrBuf + amount, wrSize - amount);

	if (amount > 0)
		file->flags &= ~O_EOF;

	if (amount < wrSize)
		return EOF;

	file->rdPtr = file->rdBuf;
	file->rdFil = file->rdBuf;

	return 0;
}

int vfsSetvbuf(FILE *file, char *buf, int mode, size_t size)
{
	if (size < 2 || mode > 2)
		return -1;

	char *ioBuf = NULL;

	if (!buf)
		ioBuf = kmalloc(size);
	else
		ioBuf = buf;

	if (file->flags & ORIGBUF)
		kfree(file->ioBuf);

	file->ioBuf = ioBuf;
	file->ioEnd = file->ioBuf + size;
	file->rdBuf = file->ioBuf;
	file->rdPtr = file->rdBuf;
	file->rdFil = file->rdBuf;
	file->rdEnd = file->rdBuf + size / 2;
	file->wrBuf = file->ioBuf + size / 2;
	file->wrPtr = file->wrBuf;
	file->wrEnd = file->ioEnd;

	if (buf)
		file->flags &= ~ORIGBUF;

	file->mode = mode;

	return 0;
}

int vfsRename(char *oldPath, char *newPath)
{
	rmPathDirectory(oldPath);
	rmPathDirectory(newPath);

	// Find file at oldPath
	vfs_node_t *node = findfile(root, oldPath);

	if (!node)
		return EOF;

	// File at newPath already exists
	if (findfile(root, newPath))
		return EOF;

	// Get directory of newPath
	char *newDir = getPathDir(newPath);

	if (!newDir)
		return EOF;

	vfs_node_t *newParent = findfile(root, newDir);

	kfree(newDir);

	// Parent directory not found
	if (!newParent)
	{
		kfree(newDir);
		return EOF;
	}

	// Parent file is not a directory
	if (!(newParent->file_desc->flags & FS_DIRECTORY))
	{
		kfree(newDir);
		return EOF;
	}

	// Change name of file at oldPath
	char *oldName = getPathFile(oldPath);
	char *newName = getPathFile(newPath);

	strcpy(node->file_desc->name, newName);

	// Create the file at newPath with the data at oldPath
	if (newParent->file_desc->mkfile(node->file_desc))
	{
		// Revert changes
		strcpy(node->file_desc->name, oldName);
		kfree(oldName);
		kfree(newName);
		return EOF;
	}
	
	// Delete the file at oldPath
	if (node->parent->file_desc->rmfile(node->file_desc))
	{
		// Revert changes
		newParent->file_desc->rmfile(node->file_desc);
		strcpy(node->file_desc->name, oldName);
		kfree(oldName);
		kfree(newName);
		return EOF;
	}

	kfree(oldName);
	kfree(newName);

	// Unlink file
	if (node->parent->child == node)
		node->parent->child = node->next;

	if (node->next) node->next->prev = node->prev;
	if (node->prev) node->prev->next = node->next;

	// Link file at new node
	node->parent = newParent;
	newParent->child->prev = node;
	node->next = newParent->child;
	newParent->child = node;

	return EOF;
}

int vfsRemove(char *path)
{
	rmPathDirectory(path);
	vfs_node_t *file = findfile(root, path);

	if (!file)
		return EOF;

	// Check if directory is empty
	if (file->file_desc->flags & FS_DIRECTORY)
	{
		// Try to read a directory entry
		DIR *dir = vfsOpendir(path);
		if (vfsReaddir(dir))
		{
			vfsCloseDir(dir);
			return EOF;
		}
	}

	if(file->parent->file_desc->rmfile(file->file_desc))
		return EOF;

	return 0;
}

int vfsMkdir(char *path)
{
	if (!createFile(root, path, FS_DIRECTORY))
		return EOF;

	return 0;
}

// Read single chars from the stream until EOF or newline
char *vfsGets(char *str, int count, FILE *stream)
{
	if (count < 1)
		return NULL;

	if (count == 1)
	{
		str[0] = '\0';
		return str;
	}

	int index = 0;
	char buf;

	while(vfsRead(stream, &buf, 1) && index < count - 1)
	{
		str[index++] = buf;
		if (buf == '\n')
			break;
	}

	if (index == 0)
		return NULL;

	str[index] = '\0';

	return str;
}

int vfsPuts(const char *str, FILE *stream)
{
	if (!str)
		return EOF;

	int index = 0;

	// Write every char
	while(str[index] != '\0')
	{
		if (!vfsWrite(stream, str + index++, 1))
			return EOF;
	}

	return 0;
}

DIR *vfsOpendir(char *path)
{
	rmPathDirectory(path);

	DIR *dir = kzalloc(sizeof(DIR));
	vfs_node_t *node = findfile(root, path);

	if (!node)
	{
		kfree(dir);
		return NULL;
	}
	
	dir->dirfile = node->file_desc;

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
