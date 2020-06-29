#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../include/display/cells.h"
#include "../../include/hal/pit.h"

#include "global.h"
#include "renderer.h"
#include "logic.h"

/*
 * Globals implementation
 */

int gamestate = STATE_MENU;

uint32_t score = 0;
uint32_t hiscores[MAX_SCORES] = { 0 };
FILE *hiscoreFile;

uint16_t snake[WIDTH * HEIGHT + 1] = { 0xFFFF };
uint16_t apple = 0;

int selected = 1;

uint32_t elapsedTime = 0;

/*
 * Private method declarations
 */

static void init();
static void exit();

/*
 * Private method implementation
 */

static void init()
{
	disableCursor();
	addSubhandler((pit_subhandler_t)render, 10);
	addSubhandler((pit_subhandler_t)update, 10);

	hiscoreFile = fopen("/scores.bin", "rb+");

	// As rb+ does not create the file and wb+ discards the contents if the file
	// already exists we first try to open it in rb+ and if it doesn't exist we
	// create it with wb+
	if (!hiscoreFile) // File does not exist
		hiscoreFile = fopen("/scores.bin", "wb+");
}

static void exit()
{
	enableCursor(0, 0);
	remSubhandler((pit_subhandler_t)render);
	remSubhandler((pit_subhandler_t)update);

	fclose(hiscoreFile);
}

/*
 * Main Method
 */

int main(int argc, char* argv[])
{
	init();

	while(gamestate != STATE_EXIT);

	exit();

	return 0;
}
