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
		return -1;
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
	pixel_t internBuffer[VID_MEM_SIZE];				//Get an intern buffer for copying purpose

	memcpy(internBuffer, memory, VID_MEM_SIZE);		//Get current display bytes to intern buffer
	memcpy(memory, buffer, VID_MEM_SIZE);			//Copy current buffer to display
	memcpy(buffer, internBuffer, VID_MEM_SIZE);		//Save previous state to buffer
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
}