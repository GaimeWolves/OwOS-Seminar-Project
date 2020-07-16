#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct file_list file_list_t;
struct file_list
{
	FILE* file;
	file_list_t* next;
};

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
//Will hold the parsed options in the future
static uint32_t flags = 0;
//Current entry in argv chain. Used for error handling
static int i = 0;
//stream list being read from
static file_list_t* first_entry = NULL;
static file_list_t* last_entry = NULL;
//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static file_list_t* file_list_create_entry(FILE* file)
{
	//Test if the file is valid
	if(!file)
		//Otherwise report an error
		//errno is set by fopen
		return NULL;

	//Setup entry
	file_list_t* entry = malloc(sizeof(file_list_t));
	entry->file = file;
	entry->next = NULL;

	//Return entry
	return entry;
}
static void file_list_push(file_list_t* entry)
{
	//Test if there is an entry already and add the new
	if(first_entry)
		last_entry->next = entry;
	else
		first_entry = entry;

	//Set it to the last
	last_entry = entry;
}
static file_list_t* file_list_pop(void)
{
	file_list_t* entry = NULL;
	//If the list is not empty grep the first entry
	if(first_entry)
	{
		entry = first_entry;
		if(entry->next)
		{
			first_entry = entry->next;
		}
		else
		{
			first_entry = NULL;
			last_entry = NULL;
		}
	}
	//Return the entry if available otherwise NULL
	return entry;
}

static void print_error(char error[])
{
	//Build error string
	size_t argument_length = strlen(error);
	char* error_string = malloc(5 + argument_length + 1); //"cat: " + argv[i+1] + '\0'
	//"cat: "
	error_string[0] = 'c';
	error_string[1] = 'a';
	error_string[2] = 't';
	error_string[3] = ':';
	error_string[4] = ' ';
	// argv
	strcpy(&error_string[5], error);
	//'\0'
	error_string[5 + argument_length] = 0;

	//Report error
	perror(error_string);

	//Free error string and return
	free(error_string);
}
static void free_file_list(void)
{
	//Get entry and free it
	file_list_t* current_entry = file_list_pop();
	do
	{
		fclose(current_entry->file);
		free(current_entry);
	} while ((current_entry = file_list_pop()));
}

static int parse_arguments(int argc, char* argv[])
{
	//Parse options
	for(; i < argc; i++)
	{
		//Option or stdin
		if(argv[i][0] == '-')
		{
			//"-" => stdin
			if(argv[i][1] == 0)
			{
				//Create list entry and add it to the list
				file_list_t* list_entry = file_list_create_entry(stdin);
				if(!list_entry)
					//If the entry is invalid report an error
					return -1;
				file_list_push(list_entry);
			}
			//"-{OPTION}" => option
			else
			{
				//FIXME: Add option parser
			}
		}
		//File
		else
		{
			//Create list entry and add it to the list
			file_list_t* list_entry = file_list_create_entry(fopen(argv[i], "r"));
			if(!list_entry)
				//If the entry is invalid report an error
				return -1;
			file_list_push(list_entry);
		}
	}

	//If there was no file given use the stdin stream
	if(!first_entry)
	{
		//Create list entry and add it to the list
		file_list_t* list_entry = file_list_create_entry(stdin);
		if(!list_entry)
			//If the entry is invalid report an error
			return -1;
		file_list_push(list_entry);
	}

	return 0;
}

static int handle_file(FILE* file)
{
	char buffer[201];
	size_t read;
	while(!feof(file))
	{
		if((read = fread(buffer, sizeof(char), 200, file)))
		{
			//Test if less than requested was read and then test for an error
			if(read != 200)
			{
				if(ferror(file))
				{
					//Print error with file name
					print_error(file->file_desc->name);
					return -1;
				}
			}

			//Add termination character because fread don't
			buffer[read] = 0;
			printf(buffer);
		}
	}
	return 0;
}
//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	//Don't pass the application name to the function
	if(parse_arguments(argc - 1, &argv[1]) != 0)
	{
		//Print error
		print_error(argv[i + 1]);
		//Free file list
		free_file_list();

		return -1;
	}

	//Write stream's content to stdout
	int returnCode;
	file_list_t* current_entry = file_list_pop();
	do
	{
		returnCode = handle_file(current_entry->file);
		fclose(current_entry->file);
		free(current_entry);
		if(returnCode != 0)
		{
			free_file_list();
			return -1;
		}
	} while ((current_entry = file_list_pop()));

	return 0;
}