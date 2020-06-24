#include "parser_main.h"

#include <string.h>

#include <memory/heap.h>
#include <shell/shell.h>
#include <shell/in_stream.h>
#include <shell/out_stream.h>
#include <shell/cwdutils.h>
#include <vfs/vfs.h>

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct argument_list argument_list_t;

struct argument_list
{
	argument_list_t* prev;
	argument_list_t* next;
	const char* str;
	size_t size;
	bool special_character;
};


//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
static shell_state_t buffer_state;
static argument_list_t* first_arg;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static inline __attribute__((always_inline)) argument_list_t* last_entry()
{
	argument_list_t* last = first_arg;
	if(!last)
		return NULL;
	while(last->next)
		last = last->next;
	return last;
}
static inline __attribute__((always_inline)) int entry_count()
{
	int count = 0;
	argument_list_t* last = first_arg;
	while(last)
	{
		count++;
		last = last->next;
	}
	return count;
}

static FILE* create_file_stream(const char* str, size_t size, char* mode)
{
	//Create a null-terminated copy
	char* file_name = kmalloc(size + 1);
	memcpy(file_name, str, size);
	file_name[size] = 0;
	//Resolve path if it is relative
	//Otherwise path is NULL
	char* path = resolve_path(file_name);
	//Free the copy
	kfree(file_name);

	//Copy mode
	char* m = kmalloc(strlen(mode) + 1);
	strcpy(m, mode);

	//Create the stream
	FILE* file = vfsOpen(path ? path : str, m);

	//Free memory
	kfree(path); //If path is NULL this just does nothing
	kfree(m);

	return file;
}

static int stream_parser(FILE** in_stream, bool* del_in_stream, FILE** out_stream, bool* del_out_stream, FILE** err_stream, bool* del_err_stream)
{
	//Loop through the linked list
	argument_list_t* current = first_arg;
	for(int i = 0; i < entry_count(); i++)
	{
		//Check if this character means stream redirection
		if(	   (current->size == 1 && (memcmp(current->str, ">", 1) == 0 || memcmp(current->str, "<", 1) == 0))
			|| (current->size == 2 && (memcmp(current->str, "2>", 2) == 0)))
			{
				//There should be an argument after that
				if(!current->next)
					return -1;
				//Detache it from the argument chain
				if(current->prev)
				{
					//This and the next should be detached
					current->prev->next = current->next->next ? current->next->next : NULL;
					if(current->prev->next)
						current->prev->next->prev = current->prev;
				}
				//Handle each stream operator
				if(current->size == 1 && (memcmp(current->str, "<", 1) == 0))
				{
					*in_stream = create_file_stream(current->next->str, current->next->size, "r");
					*del_in_stream = true;
				}
				else if(current->size == 1 && (memcmp(current->str, ">", 1) == 0))
				{
					*out_stream = create_file_stream(current->next->str, current->next->size, "w+");
					*del_out_stream = true;
				}
				else if(current->size == 2 && (memcmp(current->str, "2>", 2) == 0))
				{
					*err_stream = create_file_stream(current->next->str, current->next->size, "w+");
					*del_err_stream = true;
				}
				//Free memory
				argument_list_t* next = current->next->next;
				i--;
				kfree(current->next);
				kfree(current);
				current = next;
				continue;
			}
		//Otherwise just go to the next
		current = current->next;
	}

	//If the streams are not specified then they should be the standard streams
	if(*in_stream == NULL)
	{
		*in_stream = shell_in_stream_get();
		*del_in_stream = false;
	}
	if(*out_stream == NULL)
	{
		*out_stream = shell_out_stream_get();
		*del_out_stream = false;
	}
	if(*err_stream == NULL)
	{
		//There is no stderr stream at the moment
		//We just use the shell stdout
		*err_stream = shell_out_stream_get();
		*del_err_stream = false;
	}

	return 0;
}


//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int input_parser(const char* buffer, size_t buffersz, char** executable_name, int* argc, char*** args, FILE** in_stream, bool* del_in_stream, FILE** out_stream, bool* del_out_stream, FILE** err_stream, bool* del_err_stream)
{
	//Iterate through the buffer
	size_t length = -1;
	const char* base = buffer;
	argument_list_t* current_entry = NULL;
	for(size_t i = 0; i < buffersz; i++)
	{
		if(buffer[i] == ' ' && !buffer_state.is_quotation_mark)
		{
			if(current_entry)
			{
				current_entry->prev = last_entry();
				if(current_entry->prev)
				{
					current_entry->prev->next = current_entry;
				}
				else
				{
					first_arg = current_entry;
				}
				current_entry->str = base;
				current_entry->size = length;
				current_entry = NULL;
			}
			
			base = &buffer[i + 1];
			length = -1;
			if(buffer_state.is_escaped)
				buffer_state.is_escaped = false;
			
			continue;
		}
		if(length == (size_t)-1)
		{
			current_entry = kzalloc(sizeof(argument_list_t));
			length = 0;
		}
		if(buffer[i] == '"' && !buffer_state.is_escaped)
		{
			buffer_state.is_quotation_mark = !buffer_state.is_quotation_mark;
			length--;
			current_entry->special_character = true;
		}
		if(buffer[i] == '\\' && !buffer_state.is_escaped)
		{
			buffer_state.is_escaped = true;
			current_entry->special_character = true;
			continue;
		}
		length++;
		if(buffer_state.is_escaped)
			buffer_state.is_escaped = false;
	}
	if(current_entry)
	{
		current_entry->prev = last_entry();
		if(current_entry->prev)
		{
			current_entry->prev->next = current_entry;
		}
		else
		{
			first_arg = current_entry;
		}
		current_entry->str = base;
		current_entry->size = length;
	}

	//Get executable name
	//Get memory for executable name
	*executable_name = kmalloc(first_arg->size + 1);
	//Copy string and set the termination character
	memcpy(*executable_name, first_arg->str, first_arg->size);
	(*executable_name)[first_arg->size] = 0;

	//Check for non-absolute path
	if((*executable_name)[0] != '/')
	{
		//If we have a . then this is a file
		//Resolve the file with the pwd
		if(strstr(*executable_name, "."))
		{
			char* newBuffer = resolve_path(*executable_name);

			kfree(*executable_name);
			*executable_name = newBuffer;
		}
		//Otherwise this should be interpreted as a application which is in the /bin/ folder
		else
		{
			char* newBuffer = kmalloc(first_arg->size + 1 + 9);
			newBuffer[0] = '/';
			newBuffer[1] = 'b';
			newBuffer[2] = 'i';
			newBuffer[3] = 'n';
			newBuffer[4] = '/';
			memcpy(&newBuffer[5], *executable_name, first_arg->size);
			newBuffer[first_arg->size + 0 + 5] = '.';
			newBuffer[first_arg->size + 1 + 5] = 'e';
			newBuffer[first_arg->size + 2 + 5] = 'l';
			newBuffer[first_arg->size + 3 + 5] = 'f';
			newBuffer[first_arg->size + 4 + 5] = 0;

			kfree(*executable_name);
			*executable_name = newBuffer;
		}
	}

	//Handle stream args
	if(stream_parser(in_stream, del_in_stream, out_stream, del_out_stream, err_stream, del_err_stream) != 0)
		return -1;

	//Argument count
	*argc = entry_count();

	//Calculate args
	if(*argc != 0)
	{
		//Get space for string array
		*args = kmalloc(sizeof(char*) * *argc);

		//Loop through args
		current_entry = first_arg;
		for(int i = 0; i < *argc; i++)
		{
			//Get buffer for string
			(*args)[i] = kmalloc(current_entry->size + 1);
			//Test if there are special_characters within and handle them separate
			if(!current_entry->special_character)
			{
				//Copy string
				memcpy((*args)[i], current_entry->str, current_entry->size);
			}
			else
			{
				//Copy char after char
				for(size_t j = 0, index = 0; j < current_entry->size; index++)
				{
					//Don't copy quotation marks if their are not escaped
					if(current_entry->str[index] == '"' && current_entry->str[index - 1] != '\\')
						continue;
					//When this special character is escaped we need to overwrite the escape character
					if ((current_entry->str[index] == '"' || current_entry->str[index] == '\\') && current_entry->str[index - 1] == '\\')
						j--;
					((*args)[i])[j++] = current_entry->str[index];
				}
			}
			//The string should be null-terminated
			((*args)[i])[current_entry->size] = 0;
			//Free entry and go to next
			current_entry = current_entry->next;
			kfree(first_arg);
			first_arg = current_entry;
		}
	}

	return 0;
}