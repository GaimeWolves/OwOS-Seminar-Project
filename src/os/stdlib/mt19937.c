#include "include/mt19937.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

// Period parameters
#define N 624              // Degree of recurrence
#define M 397              // Middle word
#define A 0x9908B0DF       // Twist matrix coefficient
#define HI_MASK 0x80000000 // Most significant w-r bits
#define LO_MASK 0x7FFFFFFF // Least significant r bits

// Tempering parameters for magic bit meddling
#define B 0x9D2C5680
#define C 0xEFC60000
#define U(i) (i >> 1)
#define S(i) (i << 7)
#define T(i) (i << 15)
#define L(i) (i >> 18)

// Seeding parameters
#define SEED 5489    // Used seed if generator was not seeded
#define F 0x6C078965

//------------------------------------------------------------------------------------------
//				Static variables
//------------------------------------------------------------------------------------------

static uint32_t MT[N] = { 0 }; // Generator state
static uint32_t index = N + 1; // Current index

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static void twist();

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Generate the next N values
static void twist()
{
	for (uint32_t i = 0; i < N; i++)
	{
		uint32_t x = (MT[i] & HI_MASK) + (MT[(i + 1) % N] & LO_MASK);
		uint32_t xA = x >> 1;

		if (x % 2 != 0)
			xA ^= A;

		MT[i] = MT[(i + M) % N] ^ xA;
	}

	index = 0;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

// Initialize the generator from a seed
void mt19937_seed(uint32_t seed)
{
	index = N;
	MT[0] = seed;

	for (uint32_t i = 1; i < N; i++)
		MT[i] = (F * (MT[i - 1] ^ (MT[i - 1] >> 30)) + i);
}

// Extract tempered value from MT[index]
// calling twist every N numbers
uint32_t mt19937_rand()
{
	if (index >= N)
	{
		if (index > N)
			mt19937_seed(SEED);

		twist();
	}

	uint32_t y = MT[index++];

	y ^= U(y);
	y ^= S(y) & B;
	y ^= T(y) & C;
	y ^= L(y);

	return y;
}
