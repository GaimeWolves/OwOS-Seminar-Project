#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/shell/in_stream.h"
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
	size_t len;
	char* chars;
} row;

enum Mode mode;
FILE* in_stream;
FILE* file;
size_t numrows;
size_t cx, cy;
row* rows;

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
	/* for (int y = 0; y < 25; y++) { */
	/* 	for (int x = 0; x < 80; x++) { */
	/* 		char c = buffer[y*80 + x]; */
	/* 		addchr(x, y, c ? c : ' '); */
	/* 	} */
	/* } */
	for (size_t i = 0; i < numrows; i++) {
		for (size_t j = 0; j < rows[i].len; j++) {
			addchr(j, i*80, rows[i].chars[j]);
		}
	}
	refresh();
}

void handleKeypress() {
	char c;
	while(vfsRead(in_stream, &c, 1) == 0);
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
		}
	}
}

int readFile(char* filename) {
	FILE* file = fopen(filename, "r");
	char* buffer = malloc(2001);
	memset(buffer, 0, 2001);
	size_t read;
	while(!feof(file)) {
		if((read = fread(buffer, sizeof(char), 2000, file))) {
			if(read != 2000)
			{
				if(ferror(file))
				{
					perror(file->file_desc->name);
					return -1;
				}
			}
		}
	}
	
	// read buffer line by line
	for (size_t i = 0, last = 0; i < read && buffer[i] != 0; i++) {
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
	in_stream = shell_in_stream_get();
	while (true) {
		refreshScreen();
		handleKeypress();
	}
	return 0;
}
