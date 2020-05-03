#ifndef _VID_MEM_H
#define _VID_MEM_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include "pixel.h"

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define VIDEO_MEMORY	0xB8000			//Adress of video memory

#define MAX_COLS        80
#define MAX_ROWS        25

#define VID_MEM_SIZE	MAX_COLS * MAX_ROWS - 1

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initVidMem();
void swapBuffer();

pixel_t* getBufferPixel(uint8_t column, uint8_t row);
int setBufferPixel(uint8_t column, uint8_t row, pixel_t pixelAttributes);

pixel_t* getMemoryPixel(uint8_t column, uint8_t row);
int setMemoryPixel(uint8_t column, uint8_t row, pixel_t pixelAttributes);

int scrollBuffer(uint8_t count);
int scrollMemory(uint8_t count);

#endif