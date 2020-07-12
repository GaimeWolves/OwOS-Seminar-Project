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
	Search
};

const char* modes[] = {"-- INSERT --", "", "-- REPLACE --", ":", "/"};

typedef struct row {
	int len;
	char* chars;
} row;

typedef struct command {
	const char* c;
	int (*f)(void* arg);
} command;

typedef struct pos {
	int row;
	int col;
} pos;

enum Mode mode;
FILE* file;
char* filename;
int numrows;
int cx, cy;
int rowoff;
row* rows;
int linumWidth = 0;
char inputBuf[512];
int inputLen = 0;
bool running = true;
FILE* file;
pos* results;
int numresults;
int resultsel = -1;
int test(void* arg) {
	cx = 5;
	cy = 5;
	return 0;
}
int writeFile(void* arg) {
	fseek(file, 0, SEEK_SET);
	for (int i = 0; i < numrows; i++) {
		fwrite((void*)rows[i].chars, sizeof(char), rows[i].len, file);
		fwrite((void*)"\n", sizeof(char), 1, file);
	}
	return 0;
}
int quit(void* arg) {
	running = false;
	return 0;
}

int writeQuit(void* arg) {
	writeFile(NULL);
	quit(NULL);
	return 0;
}
command commands[] = {
	{
		"test",
		test
	},
	{
		"w",
		writeFile
	},
	{
		"q",
		quit
	},
	{
		"wq",
		writeQuit
	}

};

void setCursor(int x, int y) {
	if (y < 0) {
		y = 0;
	} else if (y > numrows-1) {
		y = numrows-1;
	}
	if ((y-rowoff) > 22) {
		rowoff = y-22;
	}
	if ((y-rowoff) < 0) {
		rowoff = y;
	}
	if (x > rows[y].len) {
		x = rows[y].len;
	}
	else if (x < 1) {
		x = 1;
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
	if (mode == Command || mode == Search) {
		addstr(1, 24, inputBuf);
	}
	// Display [sel/total] for search
	if (resultsel >= 0) {
		int n = 0;
		// Display numresults
		for (int x = numresults; x; x /= 10, n++) {
			addchr(80-n-1, 24, (x%10)+'0');
		}
		addchr(80-n-1, 24, '/');
		n++;
		// Display resultsel
		for (int x = resultsel+1; x; x /= 10, n++) {
			addchr(80-n-1, 24, (x%10)+'0');
		}
	}

	addchr(78, 23, '0');
	for (int x = ((float)(cy)/numrows)*100, n = 0; x; x /= 10, n++) {
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
	move(cx+linumWidth, cy-rowoff);
	refresh();
}

void insertChar(char c) {
	row* cur = &rows[cy];
	cur->chars = realloc(cur->chars, cur->len+1);
	memmove(cur->chars+cx, cur->chars+cx-1, cur->len-cx+1);
	cur->len++;
	cur->chars[cx-1] = c;
	cx++;
}

void deleteChar(int i) {
	row* cur = &rows[cy];
	if (i > cur->len || i < 1) {
		return;
	}
	memmove(cur->chars+i-1, cur->chars+i, cur->len-i+1);
	cur->chars = realloc(cur->chars, cur->len-1);
	cur->len--;
}

void deleteLine(int y) {
	memmove(&rows[y], &rows[y+1], sizeof(row)*(numrows-y));
	rows = realloc(rows, sizeof(row) * (numrows-1));
	numrows--;
}

void backspace() {
	if (cx == 1) {
		deleteLine(cy);
		setCursor(rows[cy-1].len+1, cy-1);
		cx++;
	}
	deleteChar(cx-1);
	cx--;
}

void inputBackspace() {
	inputLen--;
	inputBuf[inputLen] = 0;
}

int handleCommand(char* command) {
	for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
		if (strcmp(command, commands[i].c) == 0) {
			commands[i].f(NULL);
			return 0;
		}
	}
	return -1;
}

void handleSearch(char* s) {
	resultsel = 0;
	results = malloc(sizeof(pos)*512);
	int num = 0;
	for (int r = 0; r < numrows; r++) {
		for (char* p = strstr(rows[r].chars, s); p; p = strstr(p, s)) {
			results[num].row = r;
			results[num].col = p-rows[r].chars;
			num++;
			p++;
		}
	}
	numresults = num;
}

void addLine(int y) {
	rows = realloc(rows, sizeof(row) * (numrows+1));
	memmove(&rows[y+1], &rows[y], sizeof(row)*(numrows-y));
	rows[y].len = 0;
	rows[y].chars = malloc(1);
	rows[y].chars[0] = '\0';
	numrows++;
}

void deleteRange(int y, int x1, int x2) {
	row* cur = &rows[y];
	memmove(cur->chars+x1-1, cur->chars+x2, cur->len-x2+1);
	cur->len -= x2-x1+1;
	cur->chars = realloc(cur->chars, cur->len);
}

int findWhitespaceForward(char* s, int x) {
	for (int i = x; s[i]; i++) {
		if (s[i] == ' ') {
			return i;
		}
	}
	return -1;
}

int findWhitespaceBackward(char* s, int x) {
	for (int i = x; i >= 0; i--) {
		if (s[i] == ' ') {
			return i;
		}
	}
	return -1;
}

void handleKeypress() {
	char c;
	int x;
	while(fread(&c, 1, 1, stdin) == 0);
	if (mode == Insert) {
		if (c == KEY_ESCAPE) {
			mode = Normal;
			setCursor(cx, cy);
		} else if (c == 8) {
			backspace();
		} else if (c == '\n') {
			addLine(cy+1);
			setCursor(0, cy+1);
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
			case 'a':
				cx++;
				mode = Insert;
				break;
			case 'A':
				cx = rows[cy].len + 1;
				mode = Insert;
				break;
			case 'o':
				addLine(cy+1);
				setCursor(0, cy+1);
				mode = Insert;
				break;
			case 'O':
				addLine(cy);
				setCursor(0, cy);
				mode = Insert;
				break;
			case 'w':
				x = findWhitespaceForward(rows[cy].chars, cx-1);
				if (x == -1) {
					setCursor(0, cy+1);
				} else {
					setCursor(x+2, cy);
				}
				break;
			case 'b':
				setCursor(findWhitespaceBackward(rows[cy].chars, cx-3)+2, cy);
				x = findWhitespaceBackward(rows[cy].chars, cx);
				if (x == -1) {
					setCursor(0, cy-1);
				} else {
					setCursor(x+2, cy);
				}
				break;
			case 'e':
				x = findWhitespaceForward(rows[cy].chars, cx+1);
				setCursor((x>=0 ? x : rows[cy].len), cy);
				break;
			case 'd':
				while(fread(&c, 1, 1, stdin) == 0);
				switch(c) {
					case 'd':
						deleteLine(cy);
						setCursor(cx, cy);
						break;
					case 'w':
						deleteRange(cy, cx, findWhitespaceForward(rows[cy].chars, cx-1)+1);
						setCursor(cx, cy);
						break;
					case 'e':
						deleteRange(cy, cx, findWhitespaceForward(rows[cy].chars, cx-1));
						setCursor(cx, cy);
						break;
				}
				break;
			case '0':
				setCursor(0, cy);
				break;
			case '$':
				setCursor(rows[cy].len, cy);
				break;
			case '/':
				mode = Search;
				break;
			case 'n':
				if (resultsel < numresults-1) {
					resultsel++;
					setCursor(results[resultsel].col+1, results[resultsel].row);
				}
				break;
			case 'N':
				if (resultsel > 0) {
					resultsel--;
					setCursor(results[resultsel].col+1, results[resultsel].row);
				}
				break;
			case ':':
				mode = Command;
				break;
		}
	}
	else if (mode == Command) {
		if (c == '\n') {
			handleCommand(inputBuf);
			inputLen = 0;
			inputBuf[inputLen] = 0;
			mode = Normal;
		} else if (c == 8) {
			inputBackspace();
		} else {
			inputBuf[inputLen++] = c;
			inputBuf[inputLen] = 0;
		}
	}
	else if (mode == Search) {
		if (c == '\n') {
			handleSearch(inputBuf);
			inputLen = 0;
			inputBuf[inputLen] = 0;
			mode = Normal;
		} else if (c == 8) {
			inputBackspace();
		} else {
			inputBuf[inputLen++] = c;
			inputBuf[inputLen] = 0;
		}
	}
}

int readFile(char* filename) {
	file = fopen(filename, "w+");
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
	size_t last = 0;
	for (size_t i = 0; i < off && buffer[i] != 0; i++) {
		if (buffer[i] == '\n') {
			addRow(buffer+last, i-last);
			last = i+1;
		}
	}
	if (last == 0) {
		addRow("", 0);
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
	while (running) {
		refreshScreen();
		handleKeypress();
	}
	fclose(file);
	return 0;
}
