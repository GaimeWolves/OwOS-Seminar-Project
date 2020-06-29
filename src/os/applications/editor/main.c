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

const char* modes[] = {"-- INSERT --", "", "-- REPLACE --", ":"};

typedef struct row {
	int len;
	char* chars;
} row;

enum Mode mode;
FILE* file;
char* filename;
int numrows;
int cx, cy;
int rowoff;
row* rows;
int linumWidth = 0;

void setCursor(int x, int y) {
	if (x > rows[rowoff+cy].len) {
		x = rows[rowoff+cy].len;
	}
	if (x < 1) {
		x = 1;
	}
	if (y > 22) {
		y = 22;
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
	addstr(0, 23, filename);
	addstr(0, 24, modes[mode]);

	addchr(78, 23, '0');
	for (int x = ((float)(rowoff+cy)/numrows)*100, n = 0; x; x /= 10, n++) {
		addchr(78-n, 23, (x%10)+'0');
	}
	addchr(79, 23, '%');

	for (int i = 1; i <= 23; i++) {
		if (rowoff+i > numrows) {
			addchr(linumWidth-1, i-1, '~');
		}
		else {
			for (int x = rowoff+i, n = 0; x; x /= 10, n++) {
				addchr(linumWidth-n-1, i-1, (x%10)+'0');
			}
		}
	}
	for (int i = 0; i < numrows && i < 23; i++) {
		for (int j = 0; rowoff+i < numrows && j < rows[rowoff+i].len && j < 80; j++) {
			addchr(linumWidth+j+1, i, rows[rowoff+i].chars[j]);
		}
	}
	move(cx+linumWidth, cy);
	refresh();
}

void insertChar(char c) {
	row* cur = &rows[rowoff+cy];
	cur->chars = realloc(cur->chars, cur->len+1);
	memmove(cur->chars+cx, cur->chars+cx-1, cur->len-cx+1);
	cur->len++;
	cur->chars[cx-1] = c;
	cx++;
}

void deleteChar(int i) {
	row* cur = &rows[rowoff+cy];
	if (i > cur->len || i < 1) {
		return;
	}
	memmove(cur->chars+i-1, cur->chars+i, cur->len-i+1);
	cur->len--;
	cur->chars = realloc(cur->chars, cur->len-1);
}

void backspace() {
	deleteChar(cx-1);
	cx--;
}

void handleKeypress() {
	char c;
	while(fread(&c, 1, 1, stdin) == 0);
	if (mode == Insert) {
		if (c == KEY_ESCAPE) {
			mode = Normal;
		} else if (c == 8) {
			backspace();
		} else if (c != 0) {
			insertChar(c);
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
			case 'x':
				deleteChar(cx);
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
	filename = argv[1];
	if (readFile(argv[1]) != 0) {
		return -1;
	}
	mode = Normal;
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	for (size_t buf = numrows; buf; buf /= 10, linumWidth++);
	cx = 1;
	while (true) {
		refreshScreen();
		handleKeypress();
	}
	return 0;
}
