#include "vid_mem.h"
#include <string.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
pixel_t* memory;
pixel_t buffer[VID_MEM_SIZE];

//------------------------------------------------------------------------------------------
//				Interrupt Handler
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static int testBorderValid(uint8_t column, uint8_t row)
{
	if(column >= MAX_COLS || row >= MAX_COLS)
		return 0;
	return 1;
}

static inline int internScroll(uint8_t count, void* buffer)
{
	if(count >= MAX_ROWS)
		return -1;

	size_t pointCount = count * MAX_COLS;

	memmove(buffer, buffer + (VID_MEM_SIZE - pointCount) * sizeof(pixel_t), pointCount * sizeof(pixel_t));

	return 0;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initVidMem(void)
{
	memory  = (pixel_t*)VIDEO_MEMORY;	//Initialize memory buffer

	return 0;
}

void swapBuffer(void)
{
	/*
	pixel_t internBuffer[VID_MEM_SIZE];				//Get an intern buffer for copying purpose

	memcpy(internBuffer, memory, VID_MEM_SIZE * sizeof(pixel_t));		//Get current display bytes to intern buffer
	memcpy(memory, buffer, VID_MEM_SIZE * sizeof(pixel_t));			//Copy current buffer to display
	memcpy(buffer, internBuffer, VID_MEM_SIZE * sizeof(pixel_t));		//Save previous state to buffer
	*/

	memcpy(memory, buffer, sizeof(pixel_t) * VID_MEM_SIZE);
}

pixel_t* getBufferPixel(uint8_t column, uint8_t row)
{
	if(!testBorderValid(column,row))
		return NULL;

	size_t index = row * 80 + column;
	return &buffer[index];
}

int setBufferPixel(uint8_t column, uint8_t row, pixel_t pixelAttributes)
{
	if(!testBorderValid(column,row))
		return -1;

	size_t index = row * 80 + column;

	buffer[index] = pixelAttributes;

	return 0;
}

pixel_t* getMemoryPixel(uint8_t column, uint8_t row)
{
	if(!testBorderValid(column,row))
		return NULL;

	size_t index = row * 80 + column;
	return &memory[index];
}

int setMemoryPixel(uint8_t column, uint8_t row, pixel_t pixelAttributes)
{
	if(!testBorderValid(column,row))
		return -1;

	size_t index = row * 80 + column;

	memory[index] = pixelAttributes;

	return 0;
}

int scrollBuffer(uint8_t count)
{
	internScroll(count, buffer);

	return 0;
}

int scrollMemory(uint8_t count)
{
	internScroll(count, memory);

	return 0;
}