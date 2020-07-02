#include "logic.h"

#include <stdlib.h>
#include <string.h>

#include "../../include/keyboard.h"
#include "../../include/hal/pit.h"

#include "global.h"
#include "audio.h"

/*
 * Constants
 */

#define UP    1
#define LEFT  2
#define DOWN  3
#define RIGHT 4

/*
 * Private varialbes
 */

int direction = RIGHT;
int lastDirection = RIGHT;

bool justPressed = false;
bool loadedHiscores = false;

/*
 * Private method declarations
 */

static void initGame();

static void updateInGame();
static void updateMenu();
static void updateGameover();

static void placeApple();

static void readScores();
static void updateScores();

/*
 * Private method implementation
 */

static void readScores()
{
	fseek(hiscoreFile, 0, SEEK_SET);
	fread((void*)hiscores, sizeof(uint32_t), MAX_SCORES, hiscoreFile);
}

static void updateScores()
{
	fseek(hiscoreFile, 0, SEEK_SET);
	for (int i = 0; i < MAX_SCORES; i++)
	{
		if (score >= hiscores[i])
		{
			memmove(&hiscores[i + 1], &hiscores[i], MAX_SCORES - 1 - i);
			hiscores[i] = score;
			fwrite((void*)hiscores, sizeof(uint32_t), MAX_SCORES, hiscoreFile);
			return;
		}
	}
}

static void initGame()
{
	readScores();

	for (int i = 0; i < WIDTH * HEIGHT; i++)
		snake[i] = 0xFFFF;

	placeApple();

	for (int i = 0; i < 4; i++)
		snake[i] = (HEIGHT / 2) * WIDTH + WIDTH / 2 - 1 + i;

	direction = LEFT;
	score = 0;
}

static void updateMenu()
{
	if (!loadedHiscores && hiscoreFile)
	{
		loadedHiscores = true;
		readScores();
	}

	if (kbWasPressed(KEY_SPACE) || kbWasPressed(KEY_RETURN))
	{
		if (!justPressed)
		{
			playSFX(FX_SELECT);

			gamestate = selected == BUTTON_START ? STATE_INGAME : STATE_EXIT;
		
			if (gamestate == STATE_INGAME)
				initGame();

			justPressed = true;
		}
	}
	else
		justPressed = false;

	if (kbWasPressed(KEY_KP_8) && selected == BUTTON_EXIT)
		selected = BUTTON_START;

	if (kbWasPressed(KEY_KP_2) && selected == BUTTON_START)
		selected = BUTTON_EXIT;
}

static void updateInGame()
{
	// Check keys

	if (kbIsPressed(KEY_KP_8) && lastDirection != DOWN)
		direction = UP;
	else if (kbIsPressed(KEY_KP_2) && lastDirection != UP)
		direction = DOWN;
	else if (kbIsPressed(KEY_KP_4) && lastDirection != RIGHT)
		direction = LEFT;
	else if (kbIsPressed(KEY_KP_6) && lastDirection != LEFT)
		direction = RIGHT;

	if (elapsedTime % 200 != 0) // Every 200ms
		return;

	// Move snake

	lastDirection = direction;

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

	bool gotApple = newPos == apple;
	int i;

	for (i = 1; i < WIDTH * HEIGHT && snake[i] != 0xFFFF; i++)
	{
		if (snake[i + 1] != 0xFFFF && snake[i] == newPos)
		{
			playSFX(FX_LOSE);
			updateScores();
			gamestate = STATE_GAMEOVER;
		}

		int current = snake[i];
		snake[i] = lastPos;
		lastPos = current;

		if (gotApple && snake[i + 1] == 0xFFFF)
		{
			playSFX(FX_APPLE);

			snake[i + 1] = current;
			break;
		}
	}

	if (gotApple)
	{
		placeApple();
		score += i;
	}
}

// Places an apple by counting the free spaces from the top left.
// If the count is equal to the number of free squares it places the apple.
// The count is bounded by the number of free squares calculated fromt the
// length of the snake.
static void placeApple()
{
	int snakeLen = 0;
	for(; snake[snakeLen] != 0xFFFF; snakeLen++);

	int range = WIDTH * HEIGHT - snakeLen;
	int offset = rand() % range + 1;

	int current = 0;
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH * HEIGHT && snake[j] != 0xFFFF; j++)
		{
			if (snake[j] == i)
			{
				current--;
				break;
			}
		}
		current++;

		if (current == offset)
		{
			apple = i;
			break;
		}
	}
}

static void updateGameover()
{
	if (kbWasPressed(KEY_SPACE) || kbWasPressed(KEY_RETURN))
	{
		if (!justPressed)
		{
			playSFX(FX_SELECT);

			gamestate = selected == BUTTON_START ? STATE_INGAME : STATE_MENU;
		
			if (gamestate == STATE_INGAME)
				initGame();

			justPressed = true;
		}
	}
	else
		justPressed = false;
	

	if (kbWasPressed(KEY_KP_8) && selected == BUTTON_EXIT)
		selected = BUTTON_START;

	if (kbWasPressed(KEY_KP_2) && selected == BUTTON_START)
		selected = BUTTON_EXIT;
}

/*
 * Public function implementation
 */

void update()
{
	elapsedTime += 10;
	
	switch(gamestate)
	{
		case STATE_MENU:
		{
			updateMenu();
			break;
		}
		case STATE_INGAME:
		{
			updateInGame();
			break;
		}
		case STATE_GAMEOVER:
		{
			updateGameover();
			break;
		}
	}
}
