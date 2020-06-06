#include <stream/fileStream.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <string.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function
//------------------------------------------------------------------------------------------
static void openFileStream(characterStream_t* stream)
{
	if(stream->isOpened)
		return;

	fileStream_t* s = (fileStream_t*)stream;
	
	//Open file
	s->file = vfsOpen(s->file_name, "rw");

	stream->isOpened = true;
}
static void writeFileStream(characterStream_t* stream, char character)
{
	if(!stream->isOpened)
		return;

	fileStream_t* s = (fileStream_t*)stream;
	
	vfsWrite(s->file, &character, 1);
}
static char readFileStream(characterStream_t* stream)
{
	if(!stream->isOpened)
		return (char)0;

	fileStream_t* s = (fileStream_t*)stream;
	
	char character;

	vfsRead(s->file, &character, 1);

	//Return character
	return character;
}
static void closeFileStream(characterStream_t* stream)
{
	if(!stream->isOpened)
		return;
	
	fileStream_t* s = (fileStream_t*)stream;

	vfsClose(s->file);
	stream->isOpened = false;
}
static void deleteFileStream(characterStream_t* stream)
{
	fileStream_t* s = (fileStream_t*)stream;

	if(stream->isOpened)
		close(stream);

	kfree(s->file_name);
	kfree(s);
}

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------
fileStream_t* createFileStream(char* file_name)
{
	fileStream_t* s = (fileStream_t*)kmalloc(sizeof(fileStream_t));

	//Init characterStream
	s->stream.open = &openFileStream;
	s->stream.write = &writeFileStream;
	s->stream.read = &readFileStream;
	s->stream.close = &closeFileStream;
	s->stream.delete = &deleteFileStream;
	s->file_name = file_name;
	s->file = NULL;
	s->stream.isOpened = false;

	return s;
}
