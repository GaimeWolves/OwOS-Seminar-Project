#include "renderer.h"

#include "../../include/display/cells.h"
#include "../../include/hal/pit.h"

#include "global.h"

// Title for the game
static const char *title = "OwOSnake";

// Rainbow text
static int colorshift = 0;

/*
 * Private function declaration
 */

static void renderInGame();
static void toCoords(uint16_t snakePos, int *x, int *y);

/*
 * Private function implementation
 */

static void toCoords(uint16_t snakePos, int *x, int *y)
{
	*x = snakePos % WIDTH;
	*y = snakePos / WIDTH;
}

static void renderInGame()
{
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	clrscr();

	printw(1, 1, "SCORE %010u", score);
	printw(66, 1, "HI %010u", hiscore);

	// Rainbow title
	for (int i = 0; i < 8; i++)
	{
		int color = ((colorshift + i) % 12) + 1;
		color += (color > 6 ? 2 : 0);

		set_color(color, BACKGROUND_BLACK);
		addchr(36 + i, 1, title[i]);
	}
	colorshift = (colorshift + 1) % 12;

	set_color(FOREGROUND_WHITE, BACKGROUND_LIGHT_GRAY);

	for (int x = 1; x < 79; x++)
	{
		addchr(x, 3, ' ');
		addchr(x, 23, ' ');
	}

	for (int y = 3; y < 24; y++)
	{
		addchr(1, y, ' ');
		addchr(78, y, ' ');
	}

	set_color(FOREGROUND_WHITE, BACKGROUND_GREEN);

	for (int x = 0; x < WIDTH; x++)
		for(int y = 0; y < HEIGHT; y++)
			addchr(x + 2, y + 4, ' ');

	set_color(FOREGROUND_WHITE, BACKGROUND_GRAY);

	int x = 0, y = 0;
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		if (snake[i] == 0xFFFF)
			break;

		toCoords(snake[i], &x, &y);
		addchr(x + 2, y + 4, ' ');
	}

	set_color(FOREGROUND_WHITE, FOREGROUND_BLACK);

	refresh();
}

/*
 * Public function implementation
 */

void render()
{
	renderInGame();
}
