#include <shell/shell.h>

#include "../graphics/frame.h"

#include <shell/in_stream.h>
#include <shell/out_stream.h>
#include <memory/heap.h>
#include <hal/cpu.h>

#include <stdnoreturn.h>

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
void shell_handle_input(char* input)
{
	//FIXME: HANDLE INPUT
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
	char* buffer = (char*)kmalloc(SHELL_MAX_INPUT_BUFFER);
	size_t buffer_index;

	characterStream_t* in_stream = shell_in_stream_get();
	open(in_stream);

	while(true)
	{
		buffer_index = 0;
		shell_frame_print_shell_line();

		char temp_c;
		while((temp_c = read(in_stream)) != '\n')
		{
			if(temp_c == 8) //BACKSPACE
			{
				if(buffer_index)
				{
					shell_frame_handle_backspace();
					buffer_index--;
				}

				continue;
			}

			if(buffer_index < SHELL_MAX_INPUT_BUFFER)
			{
				buffer[buffer_index++] = temp_c;

				shell_frame_handle_input(temp_c);
			}
			else
			{
				//FIXME: HANDLE FULL BUFFER
				halt();
			}
		}
		shell_frame_handle_input('\n');

		shell_handle_input(buffer);
	}
}

void shell_handle_output(char c)
{
	shell_frame_handle_input(c);
}