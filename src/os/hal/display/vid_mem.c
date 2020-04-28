#include "vid_mem.h"
#include <string.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Interrupt Handler
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

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

pixel_t* getPixel(uint8_t column, uint8_t row)
{
	size_t index = row * 80 + column;
	return &buffer[index];
}