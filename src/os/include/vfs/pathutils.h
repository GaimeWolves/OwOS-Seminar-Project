#ifndef _PATHUTILS_H
#define _PATHUTILS_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stddef.h>

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

char *getPathSubstr(const char *path, size_t index);
size_t getPathLength(const char *path);
char *rmPathDirectory(char *path);
char *getPathFile(const char *path);

#endif // _PATHUTILS_H
