#include <hal/interrupt.h>
#include <hal/gdt.h>
#include <hal/pit.h>
#include <hal/display.h>
#include <hal/keyboard.h>
#include <hal/atapio.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initHAL(void)
{
	int returnCode = 0;

	returnCode += initGDT();
	returnCode += initIDT();
	returnCode += initPIT();
	returnCode += initVidMem();
	returnCode += initKeyboardHal();
	returnCode += initATA();

	return returnCode;
}
