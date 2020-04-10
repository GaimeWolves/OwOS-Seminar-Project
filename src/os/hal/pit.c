#include <hal/pit.h>
#include <hal/cpu.h>
#include <hal/interrupt.h>
#include <hal/pic.h>
#include <stddef.h>
#include <stdbool.h>
//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
static pit_subhandler_t subhandlers[MAX_SUBHANDLER];
static uint32_t subhandler_counter[MAX_SUBHANDLER];
static uint32_t subhandler_current_counter[MAX_SUBHANDLER];
static size_t subhandler_count;

//------------------------------------------------------------------------------------------
//				Interrupt Handler
//------------------------------------------------------------------------------------------
__interrupt_handler static void pit_handler(InterruptFrame_t* frame)
{
	//Test every subhandler
	for(size_t i = 0; i < MAX_SUBHANDLER; i++)
	{
		//If counter switches from 1 to 0 this time
		if(subhandler_current_counter[i] == 1)
		{
			subhandler_current_counter[i] = subhandler_counter[i];
			subhandlers[i]();
			continue;
		}
		//Decrease the counter of the subhandler
		subhandler_current_counter[i]--;
	}
	//Signals the PIC that the IRQ finished
	endOfInterrupt(IRQ_ID);
}
//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initPIT(void)
{
	outb(PORT_COMMAND,			//Send initialization sequence start command to the command register
		  IWC1_BINARY			//Use binary mode
		| IWC1_CHANNEL_0		//Use channel 0 counter (IRQ0)
		| IWC1_MODE_2			//Use mode 2 (frequncy divider)
		| IWC1_LOW_HIGH_BYTES	//Send low and high bytes 
		);

    outb(PORT_CHANNEL_0,		//Send low 16 bit counter value to channel 0 port
		COUNTER_VALUE & 0xFF	//Send low 8 bits of the word
		);
    outb(PORT_CHANNEL_0,				//Send high 16 bit counter value to channel 0 port
		(COUNTER_VALUE >> 8) & 0xFF		//Send high 8 bits of the word
		);

	//Set interrupt handler and return its return code
	return setVect(INT_ADDRESS, (interruptHandler_t)pit_handler);
}

int addSubhandler(pit_subhandler_t subhandler, uint32_t divisor)
{
	//If every subhandler is in use report error
	if(subhandler_count == MAX_SUBHANDLER)
		return -100;
	
	//Set variables for the subhandler
	subhandlers[subhandler_count] = subhandler;
	subhandler_counter[subhandler_count] = divisor;
	subhandler_current_counter[subhandler_count] = divisor;

	//Increase count of subhandler
	subhandler_count++;

	//Report success
	return 0;
}

int remSubhandler(pit_subhandler_t subhandler)
{
	//Search the handler
	bool found = false;
	size_t index;
	for(size_t i = 0; i < subhandler_count; i++)
	{
		if(subhandlers[i] == subhandler)
		{
			index = i;
			found = true;
			break;
		}
	}
	//If the handler wasn't found report error
	if(!found)
		return -10;

	clearInterruptFlag(); //Interrupts can break everything within here
	//Loop through the handlers above the removed one
	for(size_t i = index; i < subhandler_count - 1; i++)
	{
		subhandlers[i] = subhandlers[i + 1];
		subhandler_counter[i] = subhandler_counter[i + 1];
		subhandler_current_counter[i] = subhandler_current_counter[i + 1];
	}
	//Reduce count
	subhandler_count--;
	setInterruptFlag();	//We are safe again

	//Return success
	return 0;
}