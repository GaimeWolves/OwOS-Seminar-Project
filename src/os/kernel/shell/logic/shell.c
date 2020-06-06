#include <shell/shell.h>

#include "../graphics/frame.h"
#include "input_parser/parser_main.h"

#include <shell/in_stream.h>
#include <shell/out_stream.h>
#include <memory/heap.h>
#include <hal/cpu.h>
#include <vfs/vfs.h>

#include <stdnoreturn.h>
#include <stdbool.h>

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
static shell_state_t shell_state;

static char* buffer;
static size_t buffer_index;
static size_t shell_line_index;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static void shell_handle_input()
{
	//Argument count
	int argc = 0;
	//Arguments
	char** args = NULL;
	//Input stream
	characterStream_t* in_stream = NULL;
	//Delete input stream
	bool del_in_stream = false;
	//Output stream
	characterStream_t* out_stream = NULL;
	//Delete output stream
	bool del_out_stream = false;
	//Error stream
	characterStream_t* err_stream = NULL;
	//Delete error stream
	bool del_err_stream = false;
	//Executable name
	char* executable_name = NULL;

	if(input_parser(buffer, buffer_index, &executable_name, &argc, &args, &in_stream, &del_in_stream, &out_stream, &del_out_stream, &err_stream, &del_err_stream) == 0)
	{
		if(in_stream)
			open(in_stream);
		if(out_stream)
			open(out_stream);
		if(err_stream)
			open(err_stream);

		FILE* exe = vfsOpen(executable_name, "r");

		if(exe)
			;//FIXME: Start program
	}

	if(del_in_stream)
		delete(in_stream);
	if(del_out_stream)
		delete(out_stream);
	if(del_err_stream)
		delete(err_stream);

	for(size_t i = 0; i < argc; i++)
	{
		kfree(args[i]);
	}
	if(args)
		kfree(args);

	if(executable_name)
		kfree(executable_name);
}

static void shell_handle_input_normal_char(char c)
{
	if(shell_state.is_escaped)
	{
		shell_state.is_escaped = false;
	}

	if(buffer_index < SHELL_MAX_INPUT_BUFFER)
	{
		//Handle logical change
		buffer[buffer_index++] = c;
		shell_line_index++;
		//Handle graphical change
		shell_frame_handle_input(c);
	}
	else
	{
		//FIXME: HANDLE FULL BUFFER
		halt();
	}
}
static bool shell_handle_input_char(char c)
{
	bool ret = true;

	switch(c)
	{
		case 8://Backspace
			if(shell_line_index)
			{
				//Handle logical change
				buffer_index--;
				shell_line_index--;
				if(shell_state.is_escaped)
					shell_state.is_escaped = false;
				if(buffer[buffer_index] == '"' && buffer_index > 0 && buffer[buffer_index - 1] != '\\')
					shell_state.is_quotation_mark = !shell_state.is_quotation_mark;
				if(buffer_index > 0 && buffer[buffer_index - 1] == '\\')
				{
					buffer_index--;
					shell_state.is_escaped = true;
				}
					
				//Handle graphical change
				shell_frame_handle_backspace();
			}
			break;
		case '\\':
			if(!shell_state.is_escaped)
			{
				//Handle graphical change
				shell_handle_input_normal_char('\\');
				//Handle logical change
				shell_state.is_escaped = true;
				break;
			}
			else
			{
				//Handle graphical change
				shell_handle_input_normal_char('\\');
			}
			break;
		case '"'://Quotation mark
			if(!shell_state.is_escaped)
				shell_state.is_quotation_mark = !shell_state.is_quotation_mark;
			shell_handle_input_normal_char('"');
			break;
		case '\n'://Return
			//If it is excaped it just means newline in shell
			if(shell_state.is_escaped)
			{
				//Handle logical change
				shell_line_index = 0;
				buffer_index--;
				shell_state.is_escaped = false;
				//Handle graphical change
				shell_frame_handle_input('\n');
				break;
			}
			//If we are not in between of quotation marks this is the end of input
			if(!shell_state.is_quotation_mark)
			{
				//End of input
				ret = false;
				//Handle graphical change
				shell_frame_handle_input('\n');
				break;
			}
			//Otherwise it's a normal character
			shell_line_index = 0;
		default://Any character with no special meaning
			shell_handle_input_normal_char(c);
			break;
	}

	return ret;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_init(void)
{
	shell_frame_init();
	shell_in_stream_init();
	shell_out_stream_init();
}

noreturn void shell_start(void)
{
	buffer = (char*)kmalloc(SHELL_MAX_INPUT_BUFFER);

	characterStream_t* in_stream = shell_in_stream_get();
	open(in_stream);

	while(true)
	{
		//Reset buffer
		buffer_index = 0;
		shell_line_index = 0;
		//Print shell hello line
		shell_frame_print_shell_line();

		//Loop until input handler says to stop
		while(shell_handle_input_char(read(in_stream)));

		//Process input
		shell_handle_input();
	}
}

void shell_handle_output(char c)
{
	shell_frame_handle_input(c);
}