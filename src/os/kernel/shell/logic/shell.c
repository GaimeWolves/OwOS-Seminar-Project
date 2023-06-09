#include <shell/shell.h>

#include "../graphics/frame.h"
#include "input_parser/parser_main.h"

#include <shell/in_stream.h>
#include <shell/out_stream.h>
#include <ld-owos/ld-owos.h>
#include <memory/heap.h>
#include <hal/cpu.h>
#include <vfs/vfs.h>

#include <stdnoreturn.h>
#include <stdbool.h>
#include <string.h>
#include <debug.h>

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
static shell_state_t shell_state;

static int returnCode = 0;

static char* buffer;
static size_t buffer_index;
static size_t shell_line_index;

//------------------------------------------------------------------------------------------
//				Private Function Declaration
//------------------------------------------------------------------------------------------
static void shell_handle_input();
static void shell_handle_input_normal_char(char c);
static bool shell_handle_input_char(char c);
static bool shell_check_intern_program(const char* name);
static int shell_handle_intern_program(FILE* in_stream, FILE* out_stream, FILE* err_stream, const char* executable, int argc, char *argv[]);

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
	FILE* in_stream = NULL;
	//Delete input stream
	bool del_in_stream = false;
	//Output stream
	FILE* out_stream = NULL;
	//Delete output stream
	bool del_out_stream = false;
	//Error stream
	FILE* err_stream = NULL;
	//Delete error stream
	bool del_err_stream = false;
	//Executable name
	char* executable_name = NULL;

	if(input_parser(buffer, buffer_index, &executable_name, &argc, &args, &in_stream, &del_in_stream, &out_stream, &del_out_stream, &err_stream, &del_err_stream) == 0)
	{
		if(shell_check_intern_program(args[0]))
		{
			returnCode = shell_handle_intern_program(in_stream, out_stream, err_stream, args[0], argc, args);
		}
		else
		{
			FILE* exe = vfsOpen(executable_name, "r");

			if(exe)
			{
				//Save screen state
				shell_frame_save_screen_state();

				returnCode = linker_main(in_stream, out_stream, err_stream, exe, argc, args);
				
				//If the screen state changed the application used the display interface
				if(shell_frame_screen_state_changed())
					shell_frame_restore_screen_state();
			}
			else
			{
				vfsWrite(err_stream, "No such executable", 18);
				vfsFlush(err_stream);
			}
		}
	}

	if(del_in_stream)
		vfsClose(in_stream);
	else
		//If the in stream was used empty it
		shell_in_empty_buffer();	
	if(del_out_stream)
		vfsClose(out_stream);
	if(del_err_stream)
		vfsClose(err_stream);

	for(int i = 0; i < argc; i++)
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

static bool shell_check_intern_program(const char* exe)
{
	return
		memcmp(exe, "cd", 2) == 0
		|| memcmp(exe, "pwd", 3) == 0
		|| memcmp(exe, "shutdown", 8) == 0;
}
static int shell_handle_intern_program(FILE* in_stream, FILE* out_stream, FILE* err_stream, const char* exe, int argc, char *argv[])
{
	if(memcmp(exe, "cd", 2) == 0)
	{
		if(argc != 2)
		{
			vfsWrite(err_stream, "Wrong parameter count", 21);
			vfsFlush(err_stream);
			return -1;
		}

		int returnCode = change_cwd(argv[1]);
		if(returnCode == -2) //No such file or directory
		{
			vfsWrite(err_stream, "No such file or directory", 25);
			vfsFlush(err_stream);
		}
		else if(returnCode == -3) //Path too long
		{
			vfsWrite(err_stream, "The path was too long", 21);
			vfsFlush(err_stream);
		}
		
		return returnCode;
	}
	if(memcmp(exe, "pwd", 3) == 0)
	{
		vfsWrite(out_stream, cwd, strlen(cwd));
		vfsFlush(out_stream);

		return 0;
	}
	if(memcmp(exe, "shutdown", 8) == 0)
	{
		//QEMU Shutdown
		outw(0x604, 0x2000);

		//This should not happen
		return -1;
	}

	return -100;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_init(void)
{
	shell_frame_init();
	shell_in_stream_init();
	shell_out_stream_init();

	//We start at the root dir
	change_cwd("/");
}

noreturn void shell_start(void)
{
	buffer = (char*)kmalloc(SHELL_MAX_INPUT_BUFFER);

	FILE* in_stream = shell_in_stream_get();

	while(true)
	{
		//Reset buffer
		buffer_index = 0;
		shell_line_index = 0;
		//Print shell hello line
		shell_frame_print_shell_line();

		//Loop until input handler says to stop
		char character;
		do
		{
			while(vfsRead(in_stream, &character, 1) == 0)
			{
				//Don't allow EOF in the shell
				in_stream->flags &= ~O_EOF;
			}
		} while(shell_handle_input_char(character));

		//Process input
		shell_handle_input();
	}
}

void shell_handle_output(char c)
{
	shell_frame_handle_input(c);
}