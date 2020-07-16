#include "include/dirent.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "../include/vfs/vfs.h"
#include "../include/shell/cwdutils.h"

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

	char* path = resolve_path(dirname);

	if (vfsMkdir(path ? path : dirname))
	{
		free(path);
		errno = ENOSPC;
		return EOF;
	}

	free(path);
	return 0;
}

int rmdir(const char *dirname)
{
	errno = 0;
	if (!dirname)
	{
		errno = ENOENT;
		return EOF;
	}

	char* path = resolve_path(dirname);

	// Check if path points to a directory
	DIR *tmp = vfsOpendir(path ? path : dirname);
	if (!tmp)
	{
		free(path);
		errno = ENOTDIR;
		return EOF;
	}

	dirent *entry = NULL;
	while((entry = vfsReaddir(tmp)))
	{
		// The entry is neither . nor .. -> Directory is not empty
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
		{
			errno = ENOTEMPTY;
			break;
		}
	}

	vfsClosedir(tmp);

	if(errno != 0)
	{
		free(path);
		return EOF;
	}

	if (vfsRemove(path ? path : dirname))
	{
		free(path);
		errno = EEXIST;
		return EOF;
	}

	free(path);
	// Try to remove it
	return 0;
}

char* getcwd(char* buf, size_t bufsz)
{
	//The buffer needs to be valid
	if(!buf || !bufsz)
		return NULL;
	//Get length of the cwd
	size_t cwdlen = strlen(cwd);
	//The buffer needs to be big enough
	if(cwdlen >= bufsz)
	{
		errno = ENOMEM;
		return NULL;
	}
	//Copy path
	memcpy(buf, cwd, cwdlen);
	buf[cwdlen] = 0;

	return buf;
}

DIR *opendir(const char *dirname)
{
	if (!dirname)
	{
		errno = ENOENT;
		return NULL;
	}

	DIR *dir;
	char* path = resolve_path(dirname);

	if (!(dir = vfsOpendir(path ? path : dirname)))
		errno = ENOENT;

	free(path);

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
