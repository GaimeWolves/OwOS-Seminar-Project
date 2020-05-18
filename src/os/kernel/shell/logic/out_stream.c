#include <shell/out_stream.h>

#include <stream/bufferedStream.h>
#include <memory/heap.h>
#include <shell/shell.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef characterStream_t shell_out_stream_t;

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
shell_out_stream_t* shell_out_stream;

//------------------------------------------------------------------------------------------
//				shell_out_stream_t Functions
//------------------------------------------------------------------------------------------
void shell_out_stream_stream_open(characterStream_t* stream)
{
	//Don't need to be opened
	return;
}
void shell_out_stream_stream_write(characterStream_t* stream, char c)
{
	shell_handle_output(c);
}
void shell_out_stream_stream_close(characterStream_t* stream)
{
	//Don't need to be closed
	return;
}
void shell_out_stream_stream_delete(characterStream_t* stream)
{
	kfree(shell_out_stream);
	shell_out_stream = NULL;
}
//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
void shell_out_stream_create_stream(void)
{
	shell_out_stream = (shell_out_stream_t*)kmalloc(sizeof(shell_out_stream_t));

	shell_out_stream->open = &shell_out_stream_stream_open;
	shell_out_stream->write = &shell_out_stream_stream_write;
	shell_out_stream->close = &shell_out_stream_stream_close;
	shell_out_stream->delete = &shell_out_stream_stream_delete;
	shell_out_stream->isOpened = true;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_out_stream_init(void)
{
	shell_out_stream_create_stream();
}

characterStream_t* shell_out_stream_get(void)
{
	return (characterStream_t*)shell_out_stream;
}

void shell_out_stream_deinit(void)
{
	delete(shell_out_stream);
}