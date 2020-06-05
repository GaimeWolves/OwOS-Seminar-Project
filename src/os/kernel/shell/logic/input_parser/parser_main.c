#include "parser_main.h"

#include <shell/shell.h>
#include <memory/heap.h>
#include <string.h>
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


//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int input_parser(const char* buffer, size_t buffersz, char** executable_name, int* argc, char*** args, characterStream_t** in_stream, bool* del_in_stream, characterStream_t** out_stream, bool* del_out_stream, characterStream_t** err_stream, bool* del_err_stream)
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
		if(length == -1)
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
	//Delete entry out of the linked list
	if(first_arg->next)
	{
		first_arg->next->prev = NULL;
	}
	current_entry = first_arg;
	first_arg = first_arg->next;
	kfree(current_entry);

	//Argument count
	*argc = entry_count();

	//Calculate args
	if(*argc != 0)
	{
		//Get space for string array
		*args = kmalloc(sizeof(char*) * *argc);

		//Loop through args
		current_entry = first_arg;
		for(size_t i = 0; i < *argc; i++)
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