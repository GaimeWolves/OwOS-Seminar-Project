#include "cells.h"

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

	setCursor((uint8_t)x, (uint8_t)y, internColor);
}

int mvaddchr(int x, int y, char character)
{
	if(x < 0 || x >= MAX_COLS)
		return -2;
	if(y < 0 || y >= MAX_ROWS)
		return -3;

	getBufferPixel((uint8_t)x, (uint8_t)y)->character = character;

	setCursorNext(x, y);
}

int mvaddstr(int x, int y, char* str)
{
	if(x < 0 || x >= MAX_COLS)
		return -2;
	
	for(; *str != 0; str++)
	{
		if(y < 0 || y >= MAX_ROWS)
			return -3;

		getBufferPixel((uint8_t)x, (uint8_t)y)->character = *str;

		x++;
		if(x == MAX_COLS)
		{
			x = 0;
			y++;
		}
	}

	move(x, y);
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

void refresh()
{
	swapBuffer();
}