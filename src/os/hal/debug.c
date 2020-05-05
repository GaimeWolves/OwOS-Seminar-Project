#include "debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define VIDMEM 0xB8000 // Address of VGA-Memory

#define COLS 80 // Number of columns
#define ROWS 25 // Number of rows

static uint8_t xPos = 0;  // Position in row
static char color = 0x0F; // Currently used color
static bool outputOn = true;

static void scrollText();
static void putc(const char c);
static void puts(const char *s);
static void putsig();

// Scrolls the screen one row up
static void scrollText()
{
	for (size_t row = 1; row < ROWS; row++)
		memcpy((void*)(VIDMEM + (row - 1) * COLS * 2), (void*)(VIDMEM + row * COLS * 2), COLS * 2);
	memset((void*)(VIDMEM + (ROWS - 1) * COLS * 2), 0, COLS * 2);
}

// Writes a character to the screen
static void putc(const char c)
{
	if (c == '\n' || xPos >= COLS)
	{
		xPos = 0;
		scrollText();
		return;
	}

	char* pos = (char*)VIDMEM + (xPos++) * 2 + (ROWS - 1) * 2 * COLS;
	*pos++ = c;
	*pos++ = color;
}

static void puts(const char *s)
{
	if (!s)
		return;

	for (size_t i = 0; s[i]; i++)
		putc(s[i]);
}

// Writes the debug signature "[DEBUG]: "
static void putsig()
{
	char old = color;
	
	color = 0x0F; // White
	putc('[');
	color = 0x0D; // Pink
	puts("DEBUG");
	color = 0x0F;
	puts("]: ");

	color = old;
}

void toggle_debug_output(bool on)
{
	outputOn = on;
}

// Sets the printed color
void debug_set_color(char foreground, char background)
{
	color = (background << 4) | (foreground & 0x0F);
}

// Prints a string to the screen
int debug_print(const char *s)
{
	if (!outputOn)
		return 0;

	if(!s)
		return -1;

	if (xPos > 0)
	{
		scrollText();
		xPos = 0;
	}

	putsig();
	puts(s);

	return 0;
}

// Uses printf to print a formated string to the screen
int debug_printf(const char *format, ...)
{
	if (!outputOn)
		return 0;
	
	if (!format)
		return -1;

	if (xPos > 0)
	{
		scrollText();
		xPos = 0;
	}

	va_list ap;
	va_start(ap, format);

	// Set max to whole screen
	char buf[ROWS * COLS] = { 0 };
	int written = vsnprintf(buf, ROWS * COLS, format, ap);

	va_end(ap);

	debug_print(buf);

	return written;
}
