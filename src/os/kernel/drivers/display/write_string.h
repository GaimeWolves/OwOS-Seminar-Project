#ifndef _WRITE_STRING_H
#define _WRITE_STRING_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <hal/display.h>
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
//				Public Function
//------------------------------------------------------------------------------------------
int writeStringAt(char* string, uint8_t column, uint8_t row, color_t color);


#endif
