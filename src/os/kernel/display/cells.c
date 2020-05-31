#include <display/cells.h>

#include "cursor_intern.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <memory/heap.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
color_t internColor = FOREGROUND_WHITE | BACKGROUND_BLACK;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static void updateColor()
{
	for(uint8_t x = 0; x < MAX_COLS; x++)
		for(uint8_t y = 0; y < MAX_ROWS; y++)
			getBufferPixel(x, y)->color = internColor;
}
static void setCursorNext(int x, int y)
{
	x++;
	if(x == MAX_COLS)
	{
		x = 0;
		y++;
	}

	move(x, y);
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void clrscr()
{
	for(uint8_t x = 0; x < MAX_COLS; x++)
		for(uint8_t y = 0; y < MAX_ROWS; y++)
			getBufferPixel(x, y)->character = 0;
}

void set_color(char fg, char bg)
{
	internColor = fg | bg;

	updateColor();
}

int move(int x, int y)
{
	if(x < 0 || x >= MAX_COLS)
		return -2;
	if(y < 0 || y >= MAX_ROWS)
		return -3;

	setCursor((uint8_t)x, (uint8_t)y);

	return 0;
}

int mvaddchr(int x, int y, char character)
{
	addchr(x, y, character);

	setCursorNext(x, y);

	return 0;
}
int mvaddstr(int x, int y, char* str)
{
	addstr(x, y, str);

	size_t len = strlen(str);
	int cursorX = (x + len) % MAX_COLS;
	int cursorY = y + (x + len) / MAX_COLS; 
	move(cursorX, cursorY);

	return 0;
}
int mvprintw(int x, int y, char* fmt, ...)
{
	size_t len = strlen(fmt);

	char* buffer = (char*)kmalloc(len + 100);

	va_list va;
	va_start(va, fmt);

	vsnprintf(buffer, len + 100, fmt, va);

	int returnCode = mvaddstr(x, y, buffer);

	kfree(buffer);

	return returnCode;
}

int addchr(int x, int y, char character)
{
	if(x < 0 || x >= MAX_COLS)
		return -2;
	if(y < 0 || y >= MAX_ROWS)
		return -3;

	pixel_t pixelAttribute;
	pixelAttribute.character = character;
	pixelAttribute.color = internColor;
	setBufferPixel((uint8_t)x, (uint8_t)y, pixelAttribute);

	return 0;
}
int addstr(int x, int y, char* str)
{
	if(x < 0 || x >= MAX_COLS)
		return -2;

	pixel_t pixelAttribute;
	pixelAttribute.color = internColor;
	for(; *str != 0; str++)
	{
		if(y < 0 || y >= MAX_ROWS)
			return -3;

		pixelAttribute.character = *str;
		setBufferPixel((uint8_t)x, (uint8_t)y, pixelAttribute);

		x++;
		if(x == MAX_COLS)
		{
			x = 0;
			y++;
		}
	}

	return 0;
}
int printw(int x, int y, char* fmt, ...)
{
	size_t len = strlen(fmt);

	char* buffer = (char*)kmalloc(len + 100);

	va_list va;
	va_start(va, fmt);

	vsnprintf(buffer, len + 100, fmt, va);

	int returnCode = addstr(x, y, buffer);

	kfree(buffer);

	return returnCode;
}

void refresh()
{
	swapBuffer();
	refreshCursor(internColor);
}