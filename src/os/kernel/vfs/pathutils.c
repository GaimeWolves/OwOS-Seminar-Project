#include <vfs/pathutils.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <memory/heap.h>
#include <string.h>

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Get a copy from a specific part of the path
// Use kfree to free the copy from the heap
char *getPathSubstr(const char *path, size_t index)
{
	// Check if index is in bounds
	size_t maxCount = getPathLength(path);
	if (index >= maxCount)
		return NULL;

	size_t len = strlen(path);

	size_t partIndex = len;
	size_t partLen = 0;
	size_t count = 0;

	// Find area of substring
	for (size_t i = 0; i < len; i++)
	{
		if (path[i] == '/')
			count++;

		if (count == index && partIndex == len && path[i] != '/')
			partIndex = i;

		if (count > index)
		{
			partLen = i - partIndex;
			break;
		}
	}

	// If there is no trailing slash the length has to be calculated
	if (index == maxCount - 1 && partLen == 0)
		partLen = (len + 1) - partIndex;

	// Duplicate the string part
	char *part = kstrndup(&path[partIndex], partLen);
	return part;
}

// Counts how deep the path goes
size_t getPathLength(const char *path)
{
	if (path[0] == '\0')
		return 0;

	size_t count = 1; // Root directory is always present
	size_t len = strlen(path);

	for (size_t i = 0; i < len; i++)
	{
		// Check for slash
		if (path[i] == '/' && path[i + 1] != 0)
			count++;
	}

	return count;
}

// Removes the first directory entry from the path
char *rmPathDirectory(char *path)
{
	// Path already empty
	if (path[0] == '\0')
		return path;

	size_t index = 0;
	size_t len = strlen(path);

	for (size_t i = 1; i < len; i++)
	{
		if (path[i - 1] == '/')
		{
			index = i;
			break;
		}
	}

	// Last iteration (only the file remains)
	if (index == 0)
	{
		path[0] = '\0';
		return path;
	}

	// Rather use memmove as the memory regions are overlapping
	memmove((void*)path, (void*)(path + index), len - index + 1);

	return path;
}

// Gets the name of the file the path leads to
char *getPathFile(const char *path)
{
	return getPathSubstr(path, getPathLength(path) - 1);
}

char *getPathDir(const char *path)
{
	int len = strlen(path);
	for (; len >= 0; len--)
		if (path[len] == '/')
			break;

	if (len < 0)
		return NULL;

	char *dir = kstrndup(path, len);

	return dir;
}
