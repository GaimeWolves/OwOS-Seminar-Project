#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct path_list path_list_t;
struct path_list
{
	char* path;
	path_list_t* next;
};

//------------------------------------------------------------------------------------------
//				Consts
//------------------------------------------------------------------------------------------
#define FLAG_RECURSIVE 0x00000001

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
path_list_t* first;
path_list_t* last;

uint32_t flags = 0;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static path_list_t* path_list_create_entry(char* path)
{
	//Test if the file is valid
	if(!path)
		//Otherwise report an error
		//errno is set by fopen
		return NULL;

	//Setup entry
	path_list_t* entry = malloc(sizeof(path_list_t));
	entry->path = path;
	entry->next = NULL;

	//Return entry
	return entry;
}
static void path_list_push(path_list_t* entry)
{
	//Test if there is an entry already and add the new
	if(first)
		last->next = entry;
	else
		first = entry;

	//Set it to the last
	last = entry;
}
static path_list_t* path_list_pop(void)
{
	path_list_t* entry = NULL;
	//If the list is not empty grep the first entry
	if(first)
	{
		entry = first;
		if(entry->next)
		{
			first = entry->next;
		}
		else
		{
			first = NULL;
			last = NULL;
		}
	}
	//Return the entry if available otherwise NULL
	return entry;
}

static int parse_arguments(int argc, char* argv[])
{
	for(int i = 0; i < argc; i++)
	{
		//this is a path
		if(argv[i][0] != '-')
		{
			path_list_t* path_entry = path_list_create_entry(argv[i]);
			path_list_push(path_entry);
		}
		else
		{
			size_t flag_count = strlen(argv[i]) - 1;
			for(; flag_count > 0; flag_count --)
			{
				char flag = argv[i][flag_count + 1];
				if(flag == 'r')
				{
					flags |= FLAG_RECURSIVE;
				}
				else
				{
					//UNDEFINED FLAG
				}
			}
		}
	}

	if(!first)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static void dir_error(char* dir)
{
	perror(dir);
}
static void handle_dir(path_list_t* path)
{
	if(flags & FLAG_RECURSIVE)
	{
		//If the recursive flag is set we remove dirs
		if(rmdir(path->path) != 0)
		{
			dir_error(path->path);
		}
	}
	else
	{
		if(remove(path->path) != 0)
		{
			dir_error(path->path);
		}
	}
}
//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	if(parse_arguments(argc - 1, &argv[1]) != 0)
	{
		perror("rm");
		return -1;
	}

	path_list_t* current;
	while((current = path_list_pop()))
	{
		handle_dir(current);
		free(current);
	}

	return 0;
}