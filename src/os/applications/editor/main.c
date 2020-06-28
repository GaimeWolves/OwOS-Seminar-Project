#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/shell/in_stream.h"
#include "../../include/display/cells.h"
#include "../../include/keyboard.h"
#include "../../include/vfs/vfs.h"

FILE* in_stream;
FILE* file;
char* buffer;

enum Mode
{
	Insert,
	Normal,
	Replace,
	Command,
};

enum Mode mode;

void refreshScreen() {
	for (int y = 0; y < 25; y++) {
		for (int x = 0; x < 80; x++) {
			char c = buffer[y*80 + x];
			addchr(x, y, c ? c : ' ');
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
			addchr(10, 10, c);
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
	buffer = malloc(2001);
	memset(buffer, 0, 2001);
	size_t read;
	while(!feof(file)) {
		if((read = fread(buffer, sizeof(char), 2000, file))) {
			if(read != 2000)
			{
				if(ferror(file))
				{
					//Print error with file name
					perror(file->file_desc->name);
					return -1;
				}
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (readFile(argv[1]) != 0) {
		return -1;
	}
	clrscr();
	refresh();
	mode = Normal;
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	in_stream = shell_in_stream_get();
	while (true) {
		refreshScreen();
		handleKeypress();
	}
	return 0;
}
