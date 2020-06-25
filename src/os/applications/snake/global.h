#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <stdint.h>

// Gamefield sizes
#define WIDTH 76
#define HEIGHT 19

// Gamestates
#define STATE_INGAME   1
#define STATE_MENU     2
#define STATE_GAMEOVER 3
#define STATE_EXIT     4

// Menu buttons
#define BUTTON_START 1
#define BUTTON_EXIT  2

#define MAX_SCORES 8 // Maximum number of highscores to save

extern int gamestate;

// Score counters
extern uint32_t score;
extern uint32_t hiscore;

extern uint32_t hiscores[];

// Position of ingame objects
extern uint16_t snake[];
extern uint16_t apple;

// Menu options
extern int selected;

extern uint32_t elapsedTime;

#endif
