#include "cursor.h"
#include "vid_mem.h"

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
void enableCursor(uint8_t cursorStart, uint8_t cursorEnd)
{
	outb(0x3D4, 0x0A);						//Command to a VGA related address
	uint8_t maskedData = inb(0x3D5) & 0xC0;	//Get current data and mask it
	outb(0x3D5, maskedData | cursorStart);	//Write back with new cursor start pos 

	outb(0x3D4, 0x0B);						//Command to a VGA related address
	maskedData = inb(0x3D5) & 0xE0;			//Get current data and mask it
	outb(0x3D5, maskedData | cursorEnd);	//Write back with new cursor end pos
}

void disableCursor(void)
{
	outb(0x3D4, 0x0A);	//Command to a VGA related address
	outb(0x3D5, 0x20);	//Write data related to a disabled cursor
}

void setCursorPos(uint8_t column, uint8_t row)
{
	//Calculate continuous address 
	uint16_t pos = (uint16_t)row * MAX_COLS + (uint16_t)column;

	setCursorContinuousPos(pos);
}

uint8_t getCursorContinuousPos(void)
{
	//Return buffer
    uint16_t pos = 0;

	//Command to a VGA related address and
	//Read data in two 8 bit packages
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;

	//Return buffer
    return pos;
}

void setCursorContinuousPos(uint8_t pos)
{
	//Command to a VGA related address and
	//Write data in two 8 bit packages
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}