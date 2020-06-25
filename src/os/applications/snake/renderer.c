#include "renderer.h"

#include <stdbool.h>

#include "../../include/display/cells.h"
#include "../../include/hal/pit.h"

#include "global.h"

#define COLOR 1

// Title for the game
static const char *title = "OwOSnake";

// Title for the menu screen
static const char *bigTitle =
	"......#####.............#####....#####.....................##..................."
	".....#.....#...........#.....#..#.....#.....................#..................."
	".....#.....#...........#.....#..#...........................#..................."
	".....#.....#..#.....#..#.....#...#.......#.####....####.....#...##...#####......"
	".....#.....#..#.....#..#.....#....###.....#....#.......#....#.##....#.....#....."
	".....#.....#...#.#.#.. #.....#.......#....#....#...####.....##......######......"
	".....#.....#...#.#.#...#.....#........#...#....#..#....#....#.##....#..........."
	".....#.....#....#.#....#.....#..#.....#...#....#..#....#....#...#...#.....#....."
	"......#####.....#.#.....#####....#####....#....#...####.#..##....#...#####......";

static const char *gameover =
	"......#####...............................#####................................."
	".....#...................................#.....#................................"
	".....#...................................#.....#................................"
	".....#.........####....###.##....#####...#.....#..#.....#...#####...#.####......"
	".....#..###........#...#..#..#..#.....#..#.....#..#.....#..#.....#...#....#....."
	".....#.....#...####... #..#..#..######...#.....#..#.....#..######....#.........."
	".....#.....#..#....#...#..#..#..#........#.....#...#...#...#.........#.........."
	".....#.....#..#....#...#..#..#..#.....#..#.....#....#.#....#.....#...#.........."
	"......#####....####.#..#..#..#...#####....#####......#......#####...###.........";

// Rainbow text
static int colorshift = 0;
static int mask = 0;
static int timestep = 0;

/*
 * Private function declaration
 */

static void renderInGame();
static void renderMenu();
static void renderGameover();

static bool isVisible(int x, int y);

static void toCoords(uint16_t pos, int *x, int *y);

/*
 * Private function implementation
 */

static void toCoords(uint16_t pos, int *x, int *y)
{
	*x = pos % WIDTH;
	*y = pos / WIDTH;
}

static void renderInGame()
{
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	clrscr();

	for (int x = 0; x < 80; x++)
	{
		for (int y = 0; y < 25; y++)
		{
			addchr(x, y, ' ');
		}
	}

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

	set_color(FOREGROUND_WHITE, BACKGROUND_RED);

	toCoords(apple, &x, &y);
	addchr(x + 2, y + 4, ' ');

	set_color(FOREGROUND_WHITE, FOREGROUND_BLACK);
}

static void renderMenu()
{
	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	clrscr();

	for (int x = 0; x < 80; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			int color = (((colorshift / 4) + (x + y + (colorshift % 4 + 1)) / 4) % 5) + 1;
			color <<= 4;

			set_color(0, bigTitle[x + y * 80] == '#' ? (COLOR ? color : BACKGROUND_WHITE) : BACKGROUND_BLACK);
			addchr(x, y + 2, ' ');
		}
	}
	colorshift = (colorshift + 1) % (5 * 4);

	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);

	addstr(51, 16, "START GAME");
	addstr(54, 20, "EXIT");
	
	if ((elapsedTime / 250) % 2 == 0) // Every half second
	{
		if (selected == BUTTON_START)
			addchr(49, 16, '>');
		else if (selected == BUTTON_EXIT)
			addchr(52, 20, '>');
	}

	addstr(16, 13, "HIGHSCORES");

	for (int i = 0; i < MAX_SCORES; i++)
		printw(14, 15 + i, "%c. %010u", '1' + i, hiscores[i]);
}

static bool isVisible(int posX, int posY)
{
	int current = 0;

	if (mask == WIDTH * HEIGHT)
		return true;

	// Zigzag pattern
	for (int x = current / 25; x < 80 + 25; x++)
	{
		for (int y = 0; y < 25; y++)
		{
			if (x - y >= 80)
				continue;

			if (x - y < 0)
				break;

			if (++current >= mask)
				return false;

			if (posX == x - y && posY == y)
				return true;
		}
	}

	return true;
}

static void renderGameover()
{
	renderInGame();

	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	for (int x = 0; x < 80; x++)
	{
		for (int y = 0; y < 25; y++)
		{
			if (isVisible(x, y))
				addchr(x, y, ' ');
		}
	}

	for (int x = 0; x < 80; x++)
		for (int y = 2; y < 11; y++)
			if (isVisible(x, y))
			{
				set_color(0, gameover[x + (y - 2) * 80] == '#' ? BACKGROUND_WHITE : BACKGROUND_BLACK);
				addchr(x, y, ' ');
			}

	static const char *retry = "RETRY?";
	static const char *ret = "RETURN";

	set_color(FOREGROUND_WHITE, BACKGROUND_BLACK);
	for (int x = 0; x < 6; x++)
		if (isVisible(x + 37, 16))
			addchr(x + 37, 16, retry[x]);

	for (int x = 0; x < 6; x++)
		if (isVisible(x + 37, 20))
			addchr(x + 37, 20, ret[x]);

	if ((elapsedTime / 250) % 2 == 0) // Every half second
	{
		int offset = selected == BUTTON_START ? 0 : 4;

		if (isVisible(35, 16 + offset))
			addchr(35, 16 + offset, '>');
	}
}

/*
 * Public function implementation
 */

void render()
{
	if (gamestate == STATE_GAMEOVER)
	{
		if (mask < 80 * 25)
			mask += 30;
	}

	if (++timestep < 10)
		return;

	timestep = 0;

	switch(gamestate)
	{
		case STATE_MENU:
		{
			renderMenu();
			break;
		}
		case STATE_INGAME:
		{
			if (mask != 0)
				mask = 0; // Has to be zero for game over cutscene
			renderInGame();
			break;
		}
		case STATE_GAMEOVER:
		{
			renderGameover();
			break;
		}
	}

	refresh();
}
