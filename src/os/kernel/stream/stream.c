#include <stream/stream.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------
bool isReadOnly(characterStream_t* stream)
{
	return !stream->write && stream->read;
}

bool isWriteOnly(characterStream_t* stream)
{
	return stream->write && !stream->read;
}

bool isOpened(characterStream_t* stream)
{
	return stream->isOpened;
}

void open(characterStream_t* stream)
{
	stream->open(stream);
}
void write(characterStream_t* stream, char character)
{
	stream->write(stream, character);
}
char read(characterStream_t* stream)
{
	return stream->read(stream);
}
void close(characterStream_t* stream)
{
	stream->close(stream);
}
void delete(characterStream_t* stream)
{
	stream->delete(stream);
}