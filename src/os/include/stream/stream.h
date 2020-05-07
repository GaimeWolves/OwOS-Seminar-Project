#ifndef _STREAM_H
#define _STREAM_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdbool.h>
//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct characterStream_t characterStream_t;

typedef void (*openStream_t)(characterStream_t*);
typedef void (*writeStream_t)(characterStream_t*, char);
typedef char (*readStream_t)(characterStream_t*);
typedef void (*closeStream_t)(characterStream_t*);
typedef void (*deleteStream_t)(characterStream_t*);



struct characterStream_t
{
	openStream_t open;
	writeStream_t write;
	readStream_t read;
	closeStream_t close;
	deleteStream_t delete;

	bool isOpened;
};

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
bool isReadOnly(characterStream_t* stream);
bool isWriteOnly(characterStream_t* stream);
bool isOpened(characterStream_t* stream);

void open(characterStream_t* stream);
void write(characterStream_t* stream, char character);
char read(characterStream_t* stream);
void close(characterStream_t* stream);
void delete(characterStream_t* stream);

#endif // _STREAM_H
