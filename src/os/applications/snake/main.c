#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../include/display/cells.h"
#include "../../include/hal/pit.h"
#include "../../include/keyboard.h"

#include "global.h"
#include "renderer.h"

/*
 * Constants
 */

#define UP    1
#define LEFT  2
#define DOWN  3
#define RIGHT 4

/*
 * Globals implementation
 */

int gamestate = 1;

uint32_t score = 0;
uint32_t hiscore = 0;

uint16_t snake[WIDTH * HEIGHT + 1] = { 0xFFFF };
uint16_t apple = 0;

/*
 * Private varialbes
 */

int timestep = 0;
int direction = RIGHT;

/*
 * Private method declarations
 */

static void init();
static void exit();

static void initGame();

static void update();
static void updateInGame();

/*
 * Private method implementation
 */

static void init()
{
	disableCursor();

	// Timing
	addSubhandler((pit_subhandler_t)render, 100);
	addSubhandler((pit_subhandler_t)update, 10);
}

static void exit()
{
	enableCursor(0, 0);
	remSubhandler((pit_subhandler_t)render);
	remSubhandler((pit_subhandler_t)update);
}

static void initGame()
{
	for (int i = 0; i < WIDTH * HEIGHT; i++)
		snake[i] = 0xFFFF;

	apple = rand() % WIDTH * HEIGHT + 1;

	for (int i = 0; i < 4; i++)
		snake[i] = (HEIGHT / 2) * WIDTH + WIDTH / 2 - 1 + i;
}

static void update()
{
	timestep++;
	updateInGame();
}

static void updateInGame()
{
	// Check keys

	if (kbIsPressed(KEY_W))
		direction = UP;
	else if (kbIsPressed(KEY_S))
		direction = DOWN;
	else if (kbIsPressed(KEY_A))
		direction = LEFT;
	else if (kbIsPressed(KEY_D))
		direction = RIGHT;

	if (timestep != 30) // Every 30ms
		return;
	timestep = 0;

	// Move snake

	int lastPos = snake[0];
	int newPos = 0;

	switch(direction)
	{
		case UP:
		{
			if (snake[0] < WIDTH) // Top row
				snake[0] += (HEIGHT - 1) * WIDTH;
			else
				snake[0] -= WIDTH;
			break;
		}
		case DOWN:
		{
			if (snake[0] > (HEIGHT - 1) * WIDTH) // Bottom row
				snake[0] %= WIDTH;
			else
				snake[0] += WIDTH;
			break;
		}
		case LEFT:
		{
			if (snake[0] % WIDTH == 0)
				snake[0] += WIDTH - 1;
			else
				snake[0]--;
			break;
		}
		case RIGHT:
		{
			if (snake[0] % WIDTH == WIDTH - 1)
				snake[0] -= WIDTH - 1;
			else
				snake[0]++;
			break;
		}
	}

	newPos = snake[0];

	for (int i = 1; i < WIDTH * HEIGHT && snake[i] != 0xFFFF; i++)
	{
		if (snake[i + 1] != 0xFFFF && snake[i] == newPos)
		{
			// TODO: Gameover
		}

		int current = snake[i];
		snake[i] = lastPos;
		lastPos = current;
	}
}

int main(int argc, char* argv[])
{
	init();

	initGame();

	while(1) {};

	exit();

	return 0;
}
