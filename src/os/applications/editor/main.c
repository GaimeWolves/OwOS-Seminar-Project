#include <stdio.h>
#include "../../include/shell/in_stream.h"
#include "../../include/display/cells.h"
#include "../../include/keyboard.h"
#include "../../include/vfs/vfs.h"

FILE* in_stream;

enum Mode
{
	Insert,
	Normal,
	Replace,
	Command,
};

enum Mode mode;

void refreshScreen() {
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

int main(int argc, char* argv[])
{
	mode = Normal;
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	in_stream = shell_in_stream_get();
	while (true) {
		refreshScreen();
		handleKeypress();
	}
	return 0;
}
