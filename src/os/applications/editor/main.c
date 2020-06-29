#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/display/cells.h"
#include "../../include/keyboard.h"
#include "../../include/vfs/vfs.h"

enum Mode
{
	Insert,
	Normal,
	Replace,
	Command,
};

typedef struct row {
	int len;
	char* chars;
} row;

enum Mode mode;
FILE* file;
int numrows;
int cx, cy;
int rowoff;
row* rows;
int linumWidth = 0;

void setCursor(int x, int y) {
	if (x > linumWidth+rows[rowoff+cy].len) {
		x = linumWidth+rows[rowoff+cy].len;
	}
	if (x < linumWidth+1) {
		x = linumWidth+1;
	}
	if (y > 25) {
		y = 25;
		rowoff++;
	}
	else if (y > numrows-1) {
		y = numrows-1;
	}
	else if (y < 0) {
		y = 0;
		if (rowoff > 0)
			rowoff--;
	}
	cx = x;
	cy = y;
}

void addRow(char* s, size_t len) {
	rows = realloc(rows, sizeof(row) * (numrows+1));
	rows[numrows].len = len;
	rows[numrows].chars = malloc(len+1);
	memcpy(rows[numrows].chars, s, len);
	rows[numrows].chars[len] = '\0';
	numrows++;
}

void refreshScreen() {
	clrscr();
	size_t digits = 0;
	for (size_t buf = numrows; buf; buf /= 10, digits++);
	for (size_t i = 1; i < 25; i++) {
		if (i > numrows) {
			addchr(digits-1, i-1, '~');
		}
		else {
			for (size_t x = i, n = 0; x; x /= 10, n++) {
				addchr(digits-n-1, i-1, (x%10)+'0');
			}
		}
	}
	for (size_t i = 0; i < numrows; i++) {
		for (size_t j = 0; j < rows[i].len; j++) {
			addchr(digits+j+1, i, rows[i].chars[j]);
		}
	}
	move(cx, cy);
	refresh();
}

void handleKeypress() {
	char c;
	while(fread(&c, 1, 1, stdin) == 0);
	if (mode == Insert) {
		if (c == KEY_ESCAPE) {
			mode = Normal;
		} else {
			//addchr(10, 10, c);
		}
	}
	else if (mode == Normal) {
		switch(c) {
			case 'i':
				mode = Insert;
				break;
			case 'j':
				setCursor(cx, cy+1);
				break;
			case 'k':
				setCursor(cx, cy-1);
				break;
			case 'h':
				setCursor(cx-1, cy);
				break;
			case 'l':
				setCursor(cx+1, cy);
				break;
		}
	}
}

int readFile(char* filename) {
	FILE* file = fopen(filename, "r");
	char* buffer = malloc(2001);
	size_t read;
	size_t off = 0;
	while(!feof(file)) {
		if((read = fread(buffer+off, sizeof(char), 2000, file))) {
			if(read != 2000)
			{
				if(ferror(file))
				{
					perror(file->file_desc->name);
					return -1;
				}
			}
			off += read;
			buffer = realloc(buffer, 2001+off);
		}
	}
	buffer[off] = '\0';
	
	// read buffer line by line
	for (size_t i = 0, last = 0; i < off && buffer[i] != 0; i++) {
		if (buffer[i] == '\n') {
			addRow(buffer+last, i-last);
			last = i+1;
		}
	}
	free(buffer);
	return 0;
}

int main(int argc, char* argv[])
{
	if (readFile(argv[1]) != 0) {
		return -1;
	}
	mode = Normal;
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	for (size_t buf = numrows; buf; buf /= 10, linumWidth++);
	cx = linumWidth+1;
	while (true) {
		refreshScreen();
		handleKeypress();
	}
	return 0;
}
