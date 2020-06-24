#include <shell/cwdutils.h>

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
#include <string.h>
#include <memory/heap.h>
#include <vfs/vfs.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
char cwd[PATH_MAX_LENGTH];

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static int check_and_copy(const char* path)
{
	//Check if the dir exists
	DIR* dir = vfsOpendir(path);
	if(!dir)
		return -2;
	vfsClosedir(dir);
	//Set new cwd
	size_t pathlen = strlen(path);
	if(pathlen >= 255)	//We need space for '/' and '\0'
		return -3;
	memcpy(cwd, path, pathlen);

	//If the new_cwd doesn't end with a '/' we need to insert it
	if(cwd[pathlen - 1] != '/')
		cwd[pathlen++] = '/';
	//We should end with a null-terminator
	cwd[pathlen] = 0;

	return 0;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
char* resolve_path(const char* path)
{
	//Test if the path is relative
	if(path[0] == '/')
		//If not we don't need to resolve anything
		return NULL;

	//Get cwd and path lengths
	size_t cwdlen = strlen(cwd);
	size_t pathlen = strlen(path);

	//Get new buffer
	char* path_buffer = kmalloc(cwdlen + pathlen + 1);
	//Setup new buffer
	memcpy(path_buffer, cwd, cwdlen);
	memcpy(&path_buffer[cwdlen], path, pathlen);
	path_buffer[cwdlen + pathlen] = 0;

	//Return new buffer
	return path_buffer;
}

int change_cwd(const char* new_cwd)
{
	char* path;
	int returnCode;
	//If new_cwd is absolute just copy it
	if((path = resolve_path(new_cwd)) == NULL) //absolute
		returnCode = check_and_copy(new_cwd);
	else //relative
	{
		returnCode = check_and_copy(path);
		kfree(path);
	}

	return returnCode;
}