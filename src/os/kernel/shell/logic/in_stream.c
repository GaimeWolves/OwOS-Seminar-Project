#include <shell/in_stream.h>

#include <vfs/vfs.h>
#include <memory/heap.h>
#include <keyboard.h>

#include <limits.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
FILE shell_in_stream;
file_desc_t shell_in_file_desc;
characterStream_t keyboard_in;

char* shell_in_buffer;
int shell_in_bufsz = 0;
int shell_in_bufbase = 0;
int shell_in_bufcount = 0;
char shell_in_unget_c_buffer;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static int get_next_buffer_pos()
{
	if(shell_in_bufsz == shell_in_bufcount)
	{
		//FIXME: RESIZE BUFFER
	}

	return (shell_in_bufbase + shell_in_bufcount) % shell_in_bufsz;
}

static void keyboard_in_write(characterStream_t* stream, char c)
{
	shell_in_buffer[get_next_buffer_pos()] = c;
	shell_in_bufcount++;
}

static size_t shell_in_shell_read(file_desc_t *node, size_t offset, size_t size, char *buf)
{
	//Test if the caller knows that we are a character stream
	if(size != 1)
		return 0;

	//If we don't have input wait
	while (!shell_in_bufcount);

	//Read from the buffer and increase to the next char
	char character = shell_in_buffer[shell_in_bufbase++];
	shell_in_bufcount--;
	//Test for eof character
	if(character == 0x04)
	{
		return 0;
	}
	//If it is not eof return it
	*buf = character;

	return 1;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_in_stream_init(void)
{
	shell_in_buffer = (char*)kmalloc(BUFSIZ);
	shell_in_bufsz = BUFSIZ;

	keyboard_in.close = NULL;
	keyboard_in.open = NULL;
	keyboard_in.read = NULL;
	keyboard_in.unread = NULL;
	keyboard_in.write = &keyboard_in_write;
	keyboard_in.delete = NULL;
	keyboard_in.isOpened = true;

	shell_in_file_desc.flags = FS_CHRDEVICE;
	shell_in_file_desc.length = INT_MAX;
	shell_in_file_desc.inode = 0;
	shell_in_file_desc.mount = NULL;
	shell_in_file_desc.parent = NULL;
	shell_in_file_desc.openWriteStreams = 0;
	shell_in_file_desc.openReadStreams = 1;
	shell_in_file_desc.read = &shell_in_shell_read;
	shell_in_file_desc.write = NULL;
	shell_in_file_desc.readdir = NULL;
	shell_in_file_desc.findfile = NULL;
	shell_in_file_desc.mkfile = NULL;
	shell_in_file_desc.rmfile = NULL;
	shell_in_file_desc.rename = NULL;

	shell_in_stream.file_desc = &shell_in_file_desc;
	shell_in_stream.flags = O_RDONLY;
	shell_in_stream.mode = _IOFBF;
	shell_in_stream.pos = 0;
	shell_in_stream.pushback = 0;
	shell_in_stream.rdBuf = &shell_in_unget_c_buffer;
	shell_in_stream.rdPtr = &shell_in_unget_c_buffer;
	shell_in_stream.rdFil = &shell_in_unget_c_buffer;
	shell_in_stream.rdEnd = &shell_in_unget_c_buffer + 1;
	shell_in_stream.wrBuf = NULL;
	shell_in_stream.wrPtr = NULL;
	shell_in_stream.wrEnd = NULL;
	shell_in_stream.ioBuf = &shell_in_unget_c_buffer;
	shell_in_stream.ioEnd = &shell_in_unget_c_buffer + 1;

	kbSetCharacterStream(&keyboard_in);
}

FILE* shell_in_stream_get(void)
{
	return &shell_in_stream;
}

void shell_in_empty_buffer(void)
{
	shell_in_bufbase = 0;
	shell_in_bufcount = 0;
}

void shell_in_stream_deinit(void)
{
	
}