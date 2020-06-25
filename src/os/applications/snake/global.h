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

extern int gamestate;

// Score counters
extern uint32_t score;
extern uint32_t hiscore;

// Position of ingame objects
extern uint16_t snake[];
extern uint16_t apple;

#endif
