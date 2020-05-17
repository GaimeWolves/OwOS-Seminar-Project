#ifndef _VFS_H
#define _VFS_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <vfs/mbr.h>

#include <stdint.h>
#include <stddef.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define FILENAME_MAX 128 // Max filename length

#define EOF -1 // EOF indicator

// Used in seek methods
#define SEEK_SET 0 // Seek uses beginning of file
#define SEEK_CUR 1 // Seek uses current position in file
#define SEEK_END 2 // Seek uses end of file

// Buffering mode
#define _IOFBF 0 // Full buffering
#define _IOLBF 1 // Line buffering
#define _IONBF 2 // No buffering

// Flags for the FILE struct
#define O_RDONLY 0x00000001 // read-only mode
#define O_WRONLY 0x00000002 // write-only mode
#define O_RDWR   0x00000004 // read/write mode
#define O_APPEND 0x00000008 // append mode (sets write position to SEEK_END before every write)
#define O_TRUNC  0x00000010 // truncate mode (truncates the filesize to 0 essentialy rewrites file)
#define O_EOF    0x80000000 // EOF indicator flag

#define BUFSIZ 1024 // Standard complete buffer size (one sector for each buffer)

// File flags
#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_SYMLINK     0x04
#define FS_MOUNTPOINT  0x08 // Is the file an active mountpoint?

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

// Predefine structs to use inside callback typedefs
struct file_desc_t;
struct FILE;
struct dirent;

// Define callback functions reffering to the used filesystem driver
typedef int (*read_callback)(struct file_desc_t *node, size_t offset, int size, char *buf);
typedef int (*write_callback)(struct file_desc_t *node, size_t offset, int size, char *buf);
typedef struct dirent *(*readdir_callback)(struct file_desc_t *node, int index);
typedef struct file_desc_t *(*findfile_callback)(struct file_desc_t *node, char *name);
typedef struct file_desc_t *(*mkdir_callback)(struct file_desc_t *node, char *name);
typedef struct file_desc_t *(*mkfile_callback)(struct file_desc_t *node, char *name);

// Internal VFS node type
// Holds information like where the file is located
typedef struct file_desc_t
{
	char name[FILENAME_MAX]; // Filename
	uint32_t flags;          // Flags
	uint32_t length;         // File length
	uint32_t inode;          // Used in filesystem driver

	uintptr_t impl; // Used in filesystem drivers (usually pointer to additional metadata)

	struct file_desc_t *link; // Used for symlinks and mountpoints

	int openWriteDesc; // How often the file is opened in write mode (max. 1)
	int openReadDesc;  // How often the file is opened in read mode

	// Callbacks
	read_callback read;
	write_callback write;
	readdir_callback readdir;
	findfile_callback findfile;
	mkdir_callback mkdir;
	mkfile_callback mkfile;
} file_desc_t;

// External FILE type
typedef struct FILE
{
	file_desc_t *file_desc; // Internal VFS file descriptor associated with the file
	uint32_t flags;       // Flags

	size_t rdPos; // Position inside file for read operations
	size_t wrPos; // Position inside file for write operations
	
	char *rdBuf; // Read buffer base address
	char *rdPtr; // Current position in read buffer
	char *rdEnd; // End of read buffer

	char *wrBuf; // write buffer base address
	char *wrPrt; // Current position in write buffer
	char *wrEnd; // End of write buffer

	// The complete buffer gets split into one read and/or one write buffer
	// depending on the mode the file is opened in
	char *ioBuf; // Complete buffer (set by setbuf/setvbuf)
	char *ioEnd; // End of complete buffer
} FILE;

// Directory entry type
typedef struct dirent
{
	char name[FILENAME_MAX]; // Entry name
	uint32_t inode;          // Used in filesystem driver
} dirent;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

int initVFS();

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

#endif // _VFS_H
