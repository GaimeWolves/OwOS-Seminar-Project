#include "cursor_intern.h"

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
uint8_t cursor_column, cursor_row;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void setCursor(uint8_t column, uint8_t row)
{
	cursor_column = column;
	cursor_row = row;
}

void refreshCursor(color_t color)
{
	setCursorPos(cursor_column, cursor_row);
	
	pixel_t* pixelData = getMemoryPixel(cursor_column, cursor_row);
	pixelData->color = color;
}