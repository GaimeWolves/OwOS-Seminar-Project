#include <shell/out_stream.h>

#include <memory/heap.h>
#include <shell/shell.h>
#include <vfs/vfs.h>
#include <limits.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
FILE shell_out_stream;
file_desc_t shell_out_file_desc;

char* shell_out_buffer;
int shell_out_bufsz = 0;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static size_t shell_out_stream_write(file_desc_t *node, size_t offset, size_t size, char *buf)
{
	for(size_t i = 0; i < size; i++)
	{
		shell_handle_output(buf[i]);
	}

	return size;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_out_stream_init(void)
{
	shell_out_buffer = (char*)kmalloc(BUFSIZ);
	shell_out_bufsz = BUFSIZ;

	shell_out_file_desc.flags = FS_CHRDEVICE;
	shell_out_file_desc.length = INT_MAX;
	shell_out_file_desc.inode = 0;
	shell_out_file_desc.mount = NULL;
	shell_out_file_desc.parent = NULL;
	shell_out_file_desc.openWriteStreams = 1;
	shell_out_file_desc.openReadStreams = 0;
	shell_out_file_desc.read = NULL;
	shell_out_file_desc.write = &shell_out_stream_write;
	shell_out_file_desc.readdir = NULL;
	shell_out_file_desc.findfile = NULL;
	shell_out_file_desc.mkfile = NULL;
	shell_out_file_desc.rmfile = NULL;
	shell_out_file_desc.rename = NULL;

	shell_out_stream.file_desc = &shell_out_file_desc;
	shell_out_stream.flags = O_WRONLY;
	shell_out_stream.mode = _IOLBF;
	shell_out_stream.pos = 0;
	shell_out_stream.pushback = 0;
	shell_out_stream.rdBuf = NULL;
	shell_out_stream.rdPtr = NULL;
	shell_out_stream.rdFil = NULL;
	shell_out_stream.rdEnd = NULL;
	shell_out_stream.wrBuf = shell_out_buffer;
	shell_out_stream.wrPtr = shell_out_buffer;
	shell_out_stream.wrEnd = shell_out_buffer + shell_out_bufsz;
	shell_out_stream.ioBuf = shell_out_buffer;
	shell_out_stream.ioEnd = shell_out_buffer + shell_out_bufsz;
}

FILE* shell_out_stream_get(void)
{
	return &shell_out_stream;
}

void shell_out_stream_deinit(void)
{
	
}