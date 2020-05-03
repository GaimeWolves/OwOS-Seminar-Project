#include "write_string.h"

#include <string.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int writeStringAt(char* string, uint8_t column, uint8_t row, color_t color)
{
	size_t len = strlen(string);

	if(row == MAX_ROWS - 1 && column + len >= MAX_COLS)
		return -100;	//NOT ENOUGH SPACE EXCEPTION

	uint8_t tempCol = column, tempRow = row;
	for(size_t i = 0; i < len; i++)
	{
		pixel_t pixelAttribute;
		pixelAttribute.character = string[i];
		pixelAttribute.color = color;

		setBufferPixel(tempCol, tempRow, pixelAttribute);

		tempCol++;
		if(tempCol == MAX_COLS)
		{
			tempCol = 0;
			tempRow++;
		}
	}

	swapBuffer();
}