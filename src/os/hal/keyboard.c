#include <hal/keyboard.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <hal/cpu.h>
#include <hal/interrupt.h>
#include <hal/pic.h>
#include <stdbool.h>

keyboardHandler_t kbKeycodeHandler;

//------------------------------------------------------------------------------------------
//				Interrupt Handler
//------------------------------------------------------------------------------------------
__interrupt_handler static void keyboard_handler(InterruptFrame_t* frame)
{
	if (kbCtrlOutBufFull()) {
		kbKeycodeHandler(kbEncReadBuf());
	}
	
    endOfInterrupt(1); //Send the endOfInterrupt signal to the PIC
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initKeyboardHal()
{
	return setVect(KB_INTERRUPT_ADDRESS, (interruptHandler_t)keyboard_handler);
}
void setKeyboardHandle(keyboardHandler_t handler)
{
	kbKeycodeHandler = handler;
}

// Read from keyboard controller status register
uint8_t kbCtrlReadStatus()
{
	return inb(KB_CTRL_STATS_REG);
}

// Returns true if keyboard controller output buffer is full
bool kbCtrlOutBufFull()
{
	return kbCtrlReadStatus() & KB_CTRL_STATS_MASK_OUT_BUF;
}

// Returns true if keyboard controller output buffer is empty
bool kbCtrlOutBufEmpty()
{
	return (kbCtrlReadStatus() & KB_CTRL_STATS_MASK_OUT_BUF) == 0;
}

// Wait for keyboard controller output buffer to be empty
void kbCtrlOutBufWaitEmpty()
{
	while (kbCtrlOutBufFull());
}

// Wait for keyboard controller output buffer to be full
void kbCtrlOutBufWaitFull()
{
	while (kbCtrlOutBufEmpty());
}

// Returns true if keyboard controller input buffer is full
bool kbCtrlInBufFull()
{
	return kbCtrlReadStatus() & KB_CTRL_STATS_MASK_IN_BUF;
}

// Returns true if keyboard controller input buffer is empty
bool kbCtrlInBufEmpty()
{
	return (kbCtrlReadStatus() & KB_CTRL_STATS_MASK_IN_BUF) == 0;
}

// Wait for keyboard controller input buffer to be empty
void kbCtrlInBufWaitEmpty()
{
	while (kbCtrlInBufFull());
}

// Wait for keyboard controller input buffer to be full
void kbCtrlInBufWaitFull()
{
	while (kbCtrlInBufEmpty());
}

// Send command to keyboard controller
void kbCtrlSendCmd(uint8_t cmd)
{
	kbCtrlOutBufWaitEmpty(); 
	outb(KB_CTRL_CMD_REG, cmd);
}

// Enable keyboard
void kbEnable()
{
	kbCtrlSendCmd(KB_CTRL_CMD_ENABLE);
}

// Disable keyboard
void kbDisable()
{
	kbCtrlSendCmd(KB_CTRL_CMD_DISABLE);
}

// Read keyboard encoder input buffer
uint8_t kbEncReadBuf()
{
	return inb(KB_ENC_INPUT_BUF);
}

// Run keyboard self-test
bool kbSelfTest()
{
	kbCtrlSendCmd(KB_CTRL_CMD_SELF_TEST);

	kbCtrlOutBufWaitFull();

	// Test passed if output buffer is 0x55
	return (kbEncReadBuf() == 0x55) ? true : false;
}

// Send command to keyboard encoder
void kbEncSendCmd(uint8_t cmd)
{
	kbCtrlInBufWaitEmpty();
	outb(KB_ENC_CMD_REG, cmd);
}

// Set the LEDs on the keyboard
void kbSetLEDs(bool scrolllock, bool numlock, bool caps)
{
	uint8_t data = 0;
	data |= scrolllock | numlock<<1 | caps<<2;
	kbEncSendCmd(KB_ENC_CMD_SET_LED);
	kbEncSendCmd(data);
}
