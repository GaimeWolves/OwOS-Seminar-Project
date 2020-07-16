#ifndef _DIRENT_H
#define _DIRENT_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stddef.h>

#include "../../include/vfs/vfs.h"

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

DIR *opendir(const char *dirname);
int closedir(DIR *dir);
dirent *readdir(DIR *dir);

void rewinddir(DIR *dir);
void seekdir(DIR *dir, long pos);
long telldir(DIR *dir);

// These methods would be in <sys/stat.h> and <unistd.h> respectively
// but since OwOS only implements these three methods from those headers
// they will be declared in here
int mkdir(const char *dirname);
int rmdir(const char *dirname);
char* getcwd(char* buf, size_t bufsz);

#endif //_DIRENT_H
