#ifndef _VFS_H
#define _VFS_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include "mbr.h"

#include <stdint.h>
#include <stddef.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define FILENAME_MAX 128 // Max filename length

// Error codes
#define EOF       -1 // EOF indicator
#define EOVERFLOW -2 // Overflow in directory stream

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
#define O_BIN    0x00000020 // binary mode
#define O_EOF    0x80000000 // EOF indicator flag
#define F_ERROR  0x20000000 // I/O error indicator

#define BUFSIZ 1024 // Standard complete buffer size (one sector for each buffer)

// File flags
#define FS_FILE        0x01
#define FS_DIRECTORY   0x02

// Dirent flags
#define DT_DIR 0x01 // Directory
#define DT_REG 0x02 // Regular file

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

typedef size_t fpos_t; // Describes a position inside a file (needed by POSIX)

// Predefine structs to use inside callback typedefs
struct file_desc_t;
struct FILE;
struct dirent;
struct DIR;
struct mountpoint_t;

// Define callback functions reffering to the used filesystem driver
typedef size_t (*read_callback)(struct file_desc_t *node, size_t offset, size_t size, char *buf);
typedef size_t (*write_callback)(struct file_desc_t *node, size_t offset, size_t size, char *buf);
typedef int (*readdir_callback)(struct DIR *dirstream);
typedef struct file_desc_t *(*findfile_callback)(struct file_desc_t *node, char *name);
typedef int (*mkfile_callback)(struct file_desc_t *file);
typedef int (*rmfile_callback)(struct file_desc_t *file);
typedef int (*rename_callback)(struct file_desc_t *file, struct file_desc_t *newParent, char *origName);

// Internal VFS node type
// Holds information like where the file is located
typedef struct file_desc_t
{
	char name[FILENAME_MAX + 1]; // Filename
	uint32_t flags;              // Flags
	uint32_t length;             // File length
	uint32_t inode;              // Used in filesystem driver

	struct mountpoint_t *mount; // Mounted filesystem this file descriptor lies

	struct file_desc_t *parent; // Used to modify underlying directory for this file

	int openWriteStreams; // How often the file is opened in write mode (max. 1)
	int openReadStreams;  // How often the file is opened in read mode

	// Callbacks
	read_callback read;
	write_callback write;
	readdir_callback readdir;
	findfile_callback findfile;
	mkfile_callback mkfile;
	rmfile_callback rmfile;
	rename_callback rename;
} file_desc_t;

// External FILE type
typedef struct FILE
{
	file_desc_t *file_desc; // Internal VFS file descriptor associated with the file
	uint32_t flags;         // Flags
	uint8_t mode;           // Buffering mode

	fpos_t pos;      // Position inside file
	size_t pushback; // Pushback count for ungetc (at max one complete read buffer)

	char *rdBuf; // Read buffer base address
	char *rdPtr; // Current position in read buffer
	char *rdFil; // Current end of read buffer
	char *rdEnd; // Absolute end of read buffer

	char *wrBuf; // write buffer base address
	char *wrPtr; // Current position in write buffer
	char *wrEnd; // Absolute end of write buffer

	// The complete buffer gets split into one read and/or one write buffer
	// depending on the mode the file is opened in
	char *ioBuf; // Complete buffer (set by setbuf/setvbuf)
	char *ioEnd; // End of complete buffer
} FILE;

// Directory entry type
typedef struct dirent
{
	char d_name[FILENAME_MAX + 1]; // Entry name
	uint32_t d_ino;                // Used in filesystem driver
	uint8_t d_type;                // Entry type
} dirent;

// Represents a directory stream
typedef struct DIR
{
	file_desc_t *dirfile; // The directory this stream handles
	size_t index;         // The current index inside the directory
	dirent entry;         // dirent structure returned by readdir
} DIR;

// Gets called when a filesystem is requested to be unmounted and no file descriptors in it are open
typedef void (*unmount_callback)(struct mountpoint_t *mountpoint);

// Represents a mounted filesystem
typedef struct mountpoint_t
{
	file_desc_t *root;         // Root node of the mounted filesystem
	partition_t *partition;   // Partition where filesystem is located
	uintptr_t metadata;       // Pointer to unique metadata for every filesystem driver
	unmount_callback unmount; // Used to clear metadata info
} mountpoint_t;


//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

int initVFS();

FILE *vfsOpen(const char *path, const char *mode);
FILE *vfsReopen(const char *path, const char *mode, FILE *stream);
void vfsClose(FILE *file);
size_t vfsRead(FILE *file, void *buf, size_t size);
size_t vfsWrite(FILE *file, const void *buf, size_t size);

int vfsSeek(FILE *file, long offset, int origin);
int vfsFlush(FILE *file);
int vfsSetvbuf(FILE *file, char *buf, int mode, size_t size);
int vfsRename(const char *oldPath, const char *newPath);
int vfsRemove(const char *path);
int vfsMkdir(const char *path);

int vfsGetc(FILE *stream);
char *vfsGets(char *str, int count, FILE *stream);
int vfsPutc(int ch, FILE *stream);
int vfsPuts(const char *str, FILE *stream);
int vfsUngetc(int ch, FILE *stream);

DIR *vfsOpendir(const char *path);
int vfsClosedir(DIR *dir);
dirent *vfsReaddir(DIR *dir);

#endif // _VFS_H
