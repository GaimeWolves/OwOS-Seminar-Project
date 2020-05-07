#include <stream/bufferedStream.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <memory/heap.h>

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
		//FIXME: EXCEPTION HANDLING
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
