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

static vfs_node_t *findfile_helper(vfs_node_t *node, char *path);
static vfs_node_t *findfile(vfs_node_t *node, const char *path);
static vfs_node_t *createFile(vfs_node_t *node, const char *path, uint32_t flags);

static int cleanupTreeHelper(vfs_node_t *node);
static void cleanupTree();

static uint8_t parseModeString(const char *mode);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Helper function to recursively free unused nodes
static int cleanupTreeHelper(vfs_node_t *node)
{
	int ret = 0;

	// Recursively try to free subdirectories
	if (node->child)
	{
		for (vfs_node_t *child = node->child; child != NULL; child = child->next)
			ret += cleanupTreeHelper(child);
	}

	// Check if the file is being written/read
	if (node->file_desc->openReadStreams > 0 || node->file_desc->openWriteStreams > 0)
		ret = 1;

	// File is ready to be cleared (except root node)
	if (!ret && node != root)
	{
		// Unlink file
		if (node->parent)
			node->parent->child = node->next;
		
		if (node->next)
			node->next->prev = NULL;
	
		// Free used memory
		kfree(node->file_desc);
		kfree(node);
	}

	return ret;
}

// Recursively tries to free unused nodes inside the node tree
static void cleanupTree()
{
	cleanupTreeHelper(root);
}

// Helper function to recursively find a file inside the node tree
static vfs_node_t *findfile_helper(vfs_node_t *node, char *path)
{
	// Get file string
	char *file = getPathSubstr(path, 0);
	if (!node) // Path is supposed to start at root dir
	{
		if (!file || file[0] == '\0') // Path starts at root dir
		{
			rmPathDirectory(path);
			kfree(file);
			return findfile_helper(root, path);
		}
		else // Invalid path (vfs driver only accepts absolute paths)
		{
			kfree(file);
			return NULL;
		}
	}

	// Correct node found
	if (getPathLength(path) == 0)
	{
		kfree(file);
		return node;
	}

	// Is the (new) node a directory?
	if (!(node->file_desc->flags & FS_DIRECTORY))
	{
		kfree(file);
		return NULL;
	}

	// Go to next subdirectory
	rmPathDirectory(path);

	// Check if node is already in memory (recursively traverse node tree)
	for (vfs_node_t *child = node->child; child; child = child->next)
	{
		if (strcmp(child->file_desc->name, file) == 0)
		{
			kfree(file);
			return findfile_helper(child, path);
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

	// Recursively traverse the new node
	kfree(file);
	return findfile_helper(newNode, path);
}

// Recursively finds a file inside the node tree specified by the path
static vfs_node_t *findfile(vfs_node_t *node, const char *path)
{
	char *copy = kstrdup(path);
	vfs_node_t *found = findfile_helper(node, copy);
	kfree(copy);
	return found;
}

// Creates a new file at the specified path relative to the node
static vfs_node_t *createFile(vfs_node_t *node, const char *path, uint32_t flags)
{
	// Check if the origin node refers to a directory
	if (node && !(node->file_desc->flags & FS_DIRECTORY))
		return NULL;

	// File already exists
	if (findfile(node, path))
		return NULL;

	char *dirPath = getPathDir(path);
	char *filename = getPathFile(path);

	if (!filename)
	{
		if (dirPath)
			kfree(dirPath);
		return NULL;
	}

	// Find the real parent directory of the new file
	vfs_node_t *parent = NULL;
	if (!dirPath)
		parent = node;
	else
		parent = findfile(node, dirPath);
	
	if (dirPath)
		kfree(dirPath);

	if (!parent)
	{
		kfree(filename);
		return NULL;
	}

	// Create the file descriptor
	file_desc_t *file = kzalloc(sizeof(file_desc_t));
	file->flags = flags;
	file->mount = parent->file_desc->mount;
	file->parent = parent->file_desc;

	strcpy(file->name, filename);
	kfree(filename);

	// Create the new file on its filesystem
	if (parent->file_desc->mkfile(file))
	{
		kfree(file);
		return NULL;
	}

	// Create the vfs node and link it
	vfs_node_t *newNode = kzalloc(sizeof(vfs_node_t));
	newNode->file_desc = file;
	newNode->parent = parent;

	if (parent->child)
	{
		parent->child->prev = newNode;
		newNode->next = parent->child;
	}

	parent->child = newNode;

	return newNode;
}

static uint8_t parseModeString(const char *mode)
{
	uint8_t flags = 0;

	size_t len = strlen(mode);
	switch(mode[0])
	{
		case 'r':
			flags |= O_RDONLY;
			break;
		case 'w':
			flags |= O_WRONLY | O_TRUNC;
			break;
		case 'a':
			flags |= O_WRONLY | O_APPEND;
			break;
	}

	if (len == 3) // Both flags present (eg. w+b, wb+, a+b, etc.)
	{
		flags |= O_BIN;
		flags |= O_RDWR;
	}
	else if (len == 2) // Only one flag present (eg. wb or w+)
	{
		if (mode[1] == '+')
			flags |= O_RDWR;
		else
			flags |= O_BIN;
	}

	return flags;
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

	// Create mountpoint
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

// Tries to open the file specified by the path
// Tries to create the file if aplicable
FILE* vfsOpen(const char *path, const char *mode)
{
	// Check if it the file already exitst
	vfs_node_t *node = findfile(NULL, path);

	if (!node)
	{
		// Read mode requires an existing file
		if (mode[0] == 'r')
			return NULL;
		else // Create the new file
			node = createFile(NULL, path, FS_FILE);

		if (!node) // The file couldn't be created
			return NULL;
	}

	// Allocate file
	FILE *file = kzalloc(sizeof(FILE));
	file->file_desc = node->file_desc;

	// Parse mode
	file->flags = parseModeString(mode);

	// Cannot open a file in write mode more than once
	if (node->file_desc->openWriteStreams > 0 && (file->flags & (O_RDWR | O_WRONLY)))
	{
		kfree(file);
		return NULL;
	}

	// Update read/write counts
	if (file->flags & O_RDWR)
	{
		node->file_desc->openReadStreams++;
		node->file_desc->openWriteStreams++;
	}
	else if (file->flags & O_RDONLY)
		node->file_desc->openReadStreams++;
	else if (file->flags & O_WRONLY)
		node->file_desc->openWriteStreams++;

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

// Associates the stream with the file at the path
// If path is NULL tries to apply mode change to the stream
FILE *vfsReopen(const char *path, const char *mode, FILE *stream)
{
	if (!stream || !mode)
		return NULL;

	// "Close" file associated with stream
	vfsFlush(stream);

	// Update read/write counts
	if (stream->flags & O_RDWR)
	{
		stream->file_desc->openReadStreams--;
		stream->file_desc->openWriteStreams--;
	}
	else if (stream->flags & O_RDONLY)
		stream->file_desc->openReadStreams--;
	else if (stream->flags & O_WRONLY)
		stream->file_desc->openWriteStreams--;

	if (path)
	{
		// Create temporary stream
		FILE *tmp = vfsOpen(path, mode);

		if (!tmp)
			return NULL;

		// Associate stream with new file
		stream->mode = tmp->mode;
		stream->flags = tmp->flags;
		stream->pos = tmp->pos;
		stream->file_desc = tmp->file_desc;
		stream->rdPtr = stream->rdBuf;
		stream->rdFil = stream->rdBuf;
		stream->wrPtr = stream->wrBuf;
		stream->pushback = 0;

		// Free temporary stream
		kfree(tmp->ioBuf);
		kfree(tmp);
	}
	else
	{
		uint8_t newMode = parseModeString(mode);

		// Will the stream be writeable now?
		if (newMode & (O_RDWR | O_RDONLY) && !(stream->flags & (O_RDWR | O_RDONLY)))
		{
			if (stream->file_desc->openWriteStreams > 0)
				return NULL;
		}

		// Update read/write counts
		if (newMode & O_RDWR)
		{
			stream->file_desc->openReadStreams++;
			stream->file_desc->openWriteStreams++;
		}
		else if (newMode & O_RDONLY)
			stream->file_desc->openReadStreams++;
		else if (newMode & O_WRONLY)
			stream->file_desc->openWriteStreams++;

		// Clear old flags and reapply new flags
		stream->flags &= ~(O_WRONLY | O_RDONLY | O_RDWR | O_APPEND | O_TRUNC | O_BIN);
		stream->flags |= newMode; // Change mode of stream
	}

	return stream;
}

// Closes the specified file stream
void vfsClose(FILE *file)
{
	if (!file)
		return;

	// Flush unwritten data to drive
	vfsFlush(file);

	// Update read/write counts
	if (file->flags & O_RDWR)
	{
		file->file_desc->openReadStreams--;
		file->file_desc->openWriteStreams--;
	}
	else if (file->flags & O_RDONLY)
		file->file_desc->openReadStreams--;
	else if (file->flags & O_WRONLY)
		file->file_desc->openWriteStreams--;

	// Check if the buffer was allocated by this driver
	// and free it
	if (file->flags & ORIGBUF)
		kfree(file->ioBuf);

	cleanupTree(); // Cleanup node tree

	kfree(file); // Free used memory
	return;
}

// Read the specified amount of bytes into the buffer
size_t vfsRead(FILE *file, void *buf, size_t size)
{
	// Was the file opened in read mode?
	if (!(file->flags & O_RDONLY || file->flags & O_RDWR))
		return 0;

	char *buffer = (char*)buf;

	size_t read = 0;
	size_t rdSize = (size_t)(file->rdEnd - file->rdBuf);

	// Read until EOF is reached or specified amount is read
	while(!(file->flags & O_EOF && file->rdPtr == file->rdFil) && read < size)
	{
		// Empty the buffer
		for (; file->rdPtr < file->rdFil && read < size; file->rdPtr++, read++)
			buffer[read] = *file->rdPtr;

		// Revert changes to file position if pushback was used
		file->pos += file->pushback;
		file->pushback = 0;

		// Did we reach the EOF or did we read enough bytes?
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

	// Return the amount of bytes written
	return read;
}

// Writes the specified amount of bytes to the file stream
size_t vfsWrite(FILE *file, const void *buf, size_t size)
{
	// Was the file opened in write mode?
	if (!(file->flags & O_WRONLY || file->flags & O_RDWR))
		return 0;

	char *buffer = (char*)buf;

	size_t written = 0;
	size_t wrSize = (size_t)(file->wrEnd - file->wrBuf);

	// Write until user-buffer is empty
	while(written < size)
	{
		// Fill the stream-buffer
		for (; file->wrPtr < file->wrEnd && written < size; file->wrPtr++, written++)
			*file->wrPtr = buffer[written];

		// Is the user-buffer empty?
		if (written >= size)
			break;

		// Write the full buffer to the file
		size_t amount = file->file_desc->write(file->file_desc, file->pos, wrSize, file->wrBuf);
		file->pos += amount;
		file->wrPtr = file->wrBuf + (wrSize - amount);

		// Move unwritten contents to beginning of stream-buffer
		memcpy(file->wrBuf, file->wrBuf + amount, wrSize - amount);

		// Break if the filesystem couldn't write all data
		if (amount < wrSize)
			break;
	}

	// Return the number of bytes written
	return written;
}

// Sets the file stream position to an offset from the specified origin
int vfsSeek(FILE *file, long offset, int origin)
{
	size_t pos = 0;

	// First flush unwritten data
	vfsFlush(file);
	
	switch(origin)
	{
		case SEEK_CUR: // Origin = current position
			pos = file->pos;
			break;
		case SEEK_END: // Origin = EOF
			pos = file->file_desc->length;
			break;
		default:       // Origin = start of file
			break;
	}

	// Check if seek results in underflow
	if (offset < 0 && pos < (size_t)(-offset))
		pos = -offset; // To end up with zero afterwards

	pos += offset;

	file->pos = pos;

	// Reset EOF if we don't exceed the filesize
	if(pos < file->file_desc->length)
		file->flags &= ~O_EOF;

	return 0;
}

// Flush the buffer contents to the drive
int vfsFlush(FILE *file)
{
	// Reset read buffer and pushback buffer
	file->rdPtr = file->rdBuf;
	file->rdFil = file->rdBuf;
	file->pos += file->pushback; // Reset file position
	file->pushback = 0;

	// Was the file opened in write mode?
	if (!(file->flags & O_WRONLY || file->flags & O_RDWR))
		return EOF;

	// Determine amount to be written
	size_t wrSize = (size_t)(file->wrPtr - file->wrBuf);

	// Write to the drive
	size_t amount = file->file_desc->write(file->file_desc, file->pos, wrSize, file->wrBuf);
	file->pos += amount;
	file->wrPtr = file->wrBuf + (wrSize - amount);

	// Move unwritten contents to beginning
	memcpy(file->wrBuf, file->wrBuf + amount, wrSize - amount);

	// Clear EOF if something was written
	if (amount > 0)
		file->flags &= ~O_EOF;

	// Check if write error occured
	if (amount < wrSize)
		return EOF;

	return 0;
}

// Sets the internal buffer to the specified parameters
int vfsSetvbuf(FILE *file, char *buf, int mode, size_t size)
{
	if (size < 2 || mode > 2)
		return -1;

	char *ioBuf = NULL;

	if (!buf) // Use driver-managed buffer
		ioBuf = kmalloc(size);
	else      // Use user-buffer
		ioBuf = buf;

	// Free old buffer if driver-managed
	if (file->flags & ORIGBUF)
		kfree(file->ioBuf);

	// Set stream buffer properties
	file->ioBuf = ioBuf;
	file->ioEnd = file->ioBuf + size;
	file->rdBuf = file->ioBuf;
	file->rdPtr = file->rdBuf;
	file->rdFil = file->rdBuf;
	file->rdEnd = file->rdBuf + size / 2;
	file->wrBuf = file->ioBuf + size / 2;
	file->wrPtr = file->wrBuf;
	file->wrEnd = file->ioEnd;

	// Clear ORIGBUF if the buffer is user-allocated
	if (buf)
		file->flags &= ~ORIGBUF;

	file->mode = mode;

	return 0;
}

// Moves a file from the old path to the new path
int vfsRename(const char *oldPath, const char *newPath)
{
	// Find file at oldPath
	vfs_node_t *node = findfile(NULL, oldPath);

	if (!node)
		return EOF;

	// File at newPath already exists
	if (findfile(NULL, newPath))
		return EOF;

	// Get directory of newPath
	char *newDir = getPathDir(newPath);

	if (!newDir)
		return EOF;

	// Find parent directory
	vfs_node_t *newParent = findfile(NULL, newDir);

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

	// Rename file in the filesystem
	if (node->file_desc->rename(node->file_desc, newParent->file_desc, oldName))
	{
		strcpy(node->file_desc->name, oldName);
		kfree(oldName);
		kfree(newName);
		return EOF;
	}
	
	kfree(oldName);
	kfree(newName);

	// Unlink file at previous node
	if (node->parent->child == node)
		node->parent->child = node->next;

	if (node->next)
		node->next->prev = node->prev;

	if (node->prev)
		node->prev->next = node->next;

	// Link file at new node
	node->parent = newParent;
	newParent->child->prev = node;
	node->next = newParent->child;
	newParent->child = node;
	node->file_desc->parent = newParent->file_desc;

	return 0;
}

// Deletes the specified file
int vfsRemove(const char *path)
{
	// Find the file
	vfs_node_t *file = findfile(NULL, path);

	if (!file)
		return EOF;

	if (file->file_desc->openReadStreams > 0 || file->file_desc->openWriteStreams > 0)
		return EOF;

	// Check if directory is empty
	if (file->file_desc->flags & FS_DIRECTORY)
	{
		// Try to read a directory entry
		DIR *dir = vfsOpendir(path);
		if (vfsReaddir(dir)) // Returns dirent on success
		{
			vfsClosedir(dir);
			return EOF;
		}
		vfsClosedir(dir);
	}

	// Remove the file from the filesystem
	if(file->parent->file_desc->rmfile(file->file_desc))
		return EOF;

	// Unlink file
	if (file->parent->child == file)
		file->parent->child = file->next;

	if (file->next)
		file->next->prev = file->prev;

	if (file->prev)
		file->prev->next = file->next;

	kfree(file->file_desc);
	kfree(file);

	cleanupTree();

	return 0;
}

// Creates a directory at the specifed path
int vfsMkdir(const char *path)
{
	if (!createFile(NULL, path, FS_DIRECTORY))
		return EOF;

	return 0;
}

// Reads one character from the stream and returns it.
int vfsGetc(FILE *stream)
{
	if (!stream)
		return EOF;

	char buf;
	if (vfsRead(stream, &buf, 1) != 1)
		return EOF;

	return (int)buf;
}

// Writes a single character to the stream
int vfsPutc(int ch, FILE *stream)
{
	char chr = (char)ch;
	return vfsWrite(stream, &chr, 1) == 1 ? 0 : EOF;
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
	int chr = 0;

	// Read every char until newline or count - 1 chars
	while(index < count - 1 && chr != '\n')
	{
		chr = vfsGetc(stream);
		if (chr == EOF)
			return NULL;

		str[index++] = (char)chr;
	}

	if (index == 0) // Return failure if nothing is written
		return NULL;

	str[index] = '\0';

	return str;
}

// Writes a null terminated string to the file and one additional newline
int vfsPuts(const char *str, FILE *stream)
{
	if (!str)
		return EOF;

	int index = 0;

	// Write every char
	while(str[index] != '\0')
	{
		if (vfsPutc(str[index++], stream))
			return EOF;
	}

	return 0;
}

// Pushes a character back into the stream buffer
// and modifies the pushback counter accordingly
int vfsUngetc(int ch, FILE *stream)
{
	if (ch == EOF || stream->pos == 0)
		return EOF;

	size_t rdsiz = (size_t)(stream->rdEnd - stream->rdBuf);
	
	if (stream->pushback == rdsiz) // Pushback buffer is full
		return EOF;

	// Move buffer or buffer position
	if (stream->rdPtr == stream->rdBuf)
	{
		memmove(stream->rdBuf + 1, stream->rdBuf, rdsiz - 1);

		if (stream->rdFil != stream->rdEnd)
			stream->rdFil++;
	}
	else
		stream->rdPtr--;

	*stream->rdPtr = (unsigned char)ch; // "unget" character
	stream->pos--;           // Decrement stream position
	stream->flags &= ~O_EOF; // Clear EOF
	stream->pushback++;

	return ch;
}

// Opens the directory associated with the path as a directory stream
DIR *vfsOpendir(const char *path)
{
	// Find the directory inside the node tree
	DIR *dir = kzalloc(sizeof(DIR));
	vfs_node_t *node = findfile(NULL, path);

	if (!node)
	{
		kfree(dir);
		return NULL;
	}
	
	dir->dirfile = node->file_desc;

	// Check if the file pointed to by the path is a directory
	if (!(dir->dirfile->flags & FS_DIRECTORY))
	{
		kfree(dir);
		return NULL;
	}

	// Increase the number of open read streams
	dir->dirfile->openReadStreams++;

	return dir;
}

// Closes a directory stream
int vfsClosedir(DIR *dir)
{
	// Decrease the number of open read streams and free the used memory
	dir->dirfile->openReadStreams--;
	kfree(dir);

	cleanupTree();

	return 0;
}

// Read one directory entry from the directory pointed to by the dir stream
dirent *vfsReaddir(DIR *dir)
{
	// Read a directory entry
	int ret = dir->dirfile->readdir(dir);

	// Check if the error is not EOF
	if (ret != EOF)
		dir->index++;

	// Other errors mean a specific entry couldn't be read but following calls may succeed
	if (ret)
		return NULL;

	// Return the dir entry
	return &dir->entry;
}
