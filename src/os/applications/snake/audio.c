#include "audio.h"

#include "../../include/hal/pit.h"

#include "global.h"

/*
 * Typedefs
 */

typedef struct node_t
{
	uint16_t frequency;
	uint16_t length;
} node_t;

/*
 * SFX
 */

// Select sound ( C )
static const node_t sfx_select[] =
{
	{ 130, 20 }, { 0, 0 }
};

// Apple sound ( C D )
static const node_t sfx_apple[] =
{
	{ 523, 30 }, { 587, 40 },
	{ 0, 0 }
};

// Gameover sound ( C A H G A F G E F )
static const node_t sfx_gameover[] =
{
	{ 262, 75 }, { 220, 75 },
	{ 247, 75 }, { 196, 75 },
	{ 220, 75 }, { 175, 75 },
	{ 196, 75 }, { 165, 75 },
	{ 175, 250 }, { 0, 0 }
};

static const node_t *currentSFX = NULL;
static int note = -1;
static int time = 0;

/*
 * Public function implementation
 */

void playSFX(int fx)
{
	if (currentSFX)
		return;

	switch(fx)
	{
		case FX_SELECT:
		{
			currentSFX = sfx_select;
			break;
		}
		case FX_APPLE:
		{
			currentSFX = sfx_apple;
			break;
		}
		case FX_LOSE:
		{
			currentSFX = sfx_gameover;
			break;
		}
	}
}

void updateAudio()
{
	if (currentSFX)
	{
		// Guarantee first note to play
		if (note == -1)
		{
			note = 0;
			speakerPlay(currentSFX[note].frequency);
			return;
		}

		time += 5;

		if (time >= currentSFX[note].length)
		{
			time -= currentSFX[note].length;
			note++;

			if (currentSFX[note].frequency == 0 && currentSFX[note].length == 0)
			{
				time = 0;
				note = -1;
				currentSFX = NULL;

				speakerStop();

				return;
			}

			speakerPlay(currentSFX[note].frequency);
		}
	}
}
