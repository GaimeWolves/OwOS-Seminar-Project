#include <hal/keyboard.h>
#include <hal/cpu.h>
#include <stdbool.h>

// Read from keyboard controller status register
uint8_t kbCtrlReadStatus()
{
	return inb(KB_CTRL_STATS_REG);
}

// Returns true if keyboard controller buffer is full
bool kbCtrlBufFull()
{
	return kbCtrlReadStatus() & KB_CTRL_STATS_MASK_IN_BUF;
}

// Wait for keyboard controller buffer to be empty
void kbCtrlWaitEmpty()
{
	while (kbCtrlBufFull());
}

// Wait for keyboard controller buffer to be full
void kbCtrlWaitFull()
{
	while (kbCtrlBufFull());
}

// Send command to keyboard controller
void kbCtrlSendCmd (uint8_t cmd)
{
	kbCtrlWaitEmpty(); 
	outb(KB_CTRL_CMD_REG, cmd);
}

// Enable keyboard
void kbEnable ()
{
	kbCtrlSendCmd(KB_CTRL_CMD_ENABLE);
}

// Disable keyboard
void kbDisable ()
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

	kbCtrlWaitFull();

	// Test passed if output buffer is 0x55
	return (kbEncReadBuf() == 0x55) ? true : false;
}
