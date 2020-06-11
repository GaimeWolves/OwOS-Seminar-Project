#include "include/dirent.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <errno.h>

#include "../include/vfs/vfs.h"

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

int mkdir(const char *dirname)
{
	if (!dirname)
	{
		errno = ENOENT;
		return EOF;
	}

	if (vfsMkdir(dirname))
	{
		errno = ENOSPC;
		return EOF;
	}

	return 0;
}

int rmdir(const char *dirname)
{
	if (!dirname)
	{
		errno = ENOENT;
		return EOF;
	}

	// Check if path points to a directory
	DIR *tmp = vfsOpendir(dirname);
	if (!tmp)
	{
		errno = ENOTDIR;
		return EOF;
	}

	if (vfsReaddir(tmp))
		errno = ENOTEMPTY;

	vfsClosedir(tmp);

	if (vfsRemove(dirname))
	{
		errno = EEXIST;
		return EOF;
	}

	// Try to remove it
	return 0;
}

DIR *opendir(const char *dirname)
{
	if (!dirname)
	{
		errno = ENOENT;
		return NULL;
	}

	DIR *dir;
	if (!(dir = vfsOpendir(dirname)))
		errno = ENOENT;

	return dir;
}

int closedir(DIR *dir)
{
	if (!dir)
	{
		errno = EBADF;
		return EOF;
	}

	if (vfsClosedir(dir))
	{
		errno = EBADF;
		return EOF;
	}

	return 0;
}

dirent *readdir(DIR *dir)
{
	if (!dir)
	{
		errno = EBADF;
		return NULL;
	}

	dirent *entry;

	if (!(entry = vfsReaddir(dir)))
	{
		errno = ENOENT;
		return NULL;
	}

	return entry;
}

void rewinddir(DIR *dir)
{
	dir->index = 0;
}

void seekdir(DIR *dir, long pos)
{
	if (!dir)
		return;

	dir->index = pos;
}

long telldir(DIR *dir)
{
	return (long)dir->index;
}
