#ifndef _PIT_H
#define _PIT_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define PORT_CHANNEL_0      0x40
#define PORT_CHANNEL_1      0x41
#define PORT_CHANNEL_2      0x42
#define PORT_COMMAND        0x43

#define PORT_CHANNEL_2_GATE 0x61

//BCD/Binary mode
#define IWC1_BINARY         0b00000000
#define IWC1_FOUR_DIGIT_BCD 0b00000001
//Operation mode
#define IWC1_MODE_0         0b00000000	//One-time counter
#define IWC1_MODE_1         0b00000010	//One-time counter triggerable through gate input
#define IWC1_MODE_2         0b00000100	//Frequency divider
#define IWC1_MODE_3         0b00000110	//Frequency divider with square wave output
#define IWC1_MODE_4         0b00001000	//Retriggerable delay
#define IWC1_MODE_5         0b00001010	//Retriggerable delay triggerable through gate input
//Access mode
#define IWC1_LOW_BYTES      0b00010000
#define IWC1_HIGH_BYTES     0b00100000
#define IWC1_LOW_HIGH_BYTES 0b00110000
//Select PIT channel
#define IWC1_CHANNEL_0      0b00000000
#define IWC1_CHANNEL_1      0b01000000
#define IWC1_CHANNEL_2      0b10000000

#define FREQUENCY			1000			//1ms
#define COUNTER_VALUE		1193182 / FREQUENCY
#define IRQ_ID				0
#define INT_ADDRESS         32 + IRQ_ID

#define MAX_SUBHANDLER		5

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef void (*pit_subhandler_t)(void);

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initPIT(void);

int addSubhandler(pit_subhandler_t,uint32_t);
int remSubhandler(pit_subhandler_t);

void speakerPlay(uint32_t frequency);
void speakerStop();

#endif
