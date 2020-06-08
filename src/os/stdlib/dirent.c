#include "include/dirent.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include "../include/vfs/vfs.h"

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

int mkdir(const char *dirname)
{
	return vfsMkdir(dirname);
}

int rmdir(const char *dirname)
{
	// Check if path points to a directory
	DIR *tmp = vfsOpendir(dirname);
	if (!tmp)
		return EOF;
	vfsClosedir(tmp);

	// Try to remove it
	return vfsRemove(dirname);
}

DIR *opendir(const char *dirname)
{
	return vfsOpendir(dirname);
}

int closedir(DIR *dir)
{
	return vfsClosedir(dir);
}

dirent *readdir(DIR *dir)
{
	return vfsReaddir(dir);
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
