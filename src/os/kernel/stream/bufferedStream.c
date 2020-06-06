#include <stream/bufferedStream.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <memory/heap.h>
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
static inline size_t getNextIndex(bufferedStream_t* s)
{
	return (s->bufferBase + s->count) % s->bufferSize;
}

static void openBufferedStream(characterStream_t* stream)
{
	if(stream->isOpened)
		return;

	bufferedStream_t* s = (bufferedStream_t*)stream;
	
	//Init bufferedStream 
	s->buffer = (char*)kmalloc(BUFFER_SIZE);
	s->bufferBase = 0;
	s->bufferSize = BUFFER_SIZE;
	s->count = 0;

	stream->isOpened = true;
}
static void writeBufferedStream(characterStream_t* stream, char character)
{
	if(!stream->isOpened)
		return;

	bufferedStream_t* s = (bufferedStream_t*)stream;
	
	if(s->count < s->bufferSize)
	{
		s->buffer[getNextIndex(s)] = character;
		s->count++;
	}
	else
	{
		//Get new buffer with more size
		char* newBuffer = (char*)kmalloc(s->bufferSize * 2);

		//Copy data
		if(s->bufferBase != 0)
		{
			size_t toEnd = s->bufferSize - s->bufferBase;
			size_t fromStart = s->count - toEnd;
			memcpy(newBuffer, (s->buffer + s->bufferBase), toEnd);
			memcpy(newBuffer + toEnd, s->buffer, fromStart);
		}
		else
		{
			memcpy(newBuffer, s->buffer + s->bufferBase, s->count);
		}

		//Free old buffer
		kfree(s->buffer);
		//Set vars to new buffer
		s->buffer = newBuffer;
		s->bufferSize *= 2;
		s->bufferBase = 0;

		//Add character
		s->buffer[s->count] = character;
		s->count++;
	}
}
static char readBufferedStream(characterStream_t* stream)
{
	if(!stream->isOpened)
		return (char)0;

	bufferedStream_t* s = (bufferedStream_t*)stream;
	
	//Block while buffer is empty
	while(s->count == 0);

	//Get character at the current base index
	char character = s->buffer[s->bufferBase];

	//Increase base index and decrease count
	s->bufferBase = (s->bufferBase + 1) % s->bufferSize;
	s->count--;

	//Return character
	return character;
}
static void closeBufferedStream(characterStream_t* stream)
{
	if(!stream->isOpened)
		return;
	
	bufferedStream_t* s = (bufferedStream_t*)stream;

	stream->isOpened = false;
	
	kfree(s->buffer);
}
static void deleteBufferedStream(characterStream_t* stream)
{
	bufferedStream_t* s = (bufferedStream_t*)stream;

	if(stream->isOpened)
		close(stream);

	kfree(s);
}

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------
bufferedStream_t* createBufferedStream(void)
{
	bufferedStream_t* s = (bufferedStream_t*)kmalloc(sizeof(bufferedStream_t));

	//Init characterStream
	s->stream.open = &openBufferedStream;
	s->stream.write = &writeBufferedStream;
	s->stream.read = &readBufferedStream;
	s->stream.close = &closeBufferedStream;
	s->stream.delete = &deleteBufferedStream;
	s->stream.isOpened = false;

	return s;
}
