#include <string.h>
#include <keyboard.h>
#include <hal/keyboard.h>
#include "bitmap.h"

// Normal keys
const char keys_nomod[] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0xE1, '\'', 0, 0,          // 0x00 - 0x0F
	'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', 0x81, '+', 0, 0, 'a', 's',       // 0x10 - 0x1F
	'd', 'f', 'g', 'h', 'j', 'k', 'l', 0x94, 0x84, '^', 0, '#', 'y', 'x', 'c', 'v',    // 0x20 - 0x2F
	'b', 'n', 'm', ',', '.', '-', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,                    // 0x30 - 0x3F
	0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',                  // 0x40 - 0x4F
	'2', '3', '0', ',', 0, 0, '<', 0, 0                                                // 0x50 - 0x58
};

// Keys with shift modifier
const char keys_shift[] = {
	0, 0, '!', '\"', 0, '$', '%', '&', '/', '(', ')', '=', '?', '`', 0, 0,             // 0x00 - 0x0F
	'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', 0x9A, '*', 0, 0, 'A', 'S',       // 0x10 - 0x1F
	'D', 'F', 'G', 'H', 'J', 'K', 'L', 0x99, 0x8E, 0xF8, 0, '\'', 'Y', 'X', 'C', 'V',  // 0x20 - 0x2F
	'B', 'N', 'M', ';', ':', '_', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,                    // 0x30 - 0x3F
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,                                // 0x40 - 0x4F
	0, 0, 0, 0, 0, 0, '>', 0, 0                                                        // 0x50 - 0x58
};

// Keys with control and alt modifier
const char keys_ctrl_alt[] = {
	0, 0, 0, 0xFD, 0, 0xAC, 0xAB, 0xAA, '{', '[', ']', '}', '\\', 0, 0, 0,             // 0x00 - 0x0F
	'@', 0, 0, 0x9E, 0, 0, 0, 0, 0, 0, 0, '~', 0, 0, 0x91, 0,                          // 0x10 - 0x1F
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xAF, 0xAE, 0x9B, 0,                           // 0x20 - 0x2F
	0, 0, 0xE6, 0xF9, 0, 0xC4, 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,                       // 0x30 - 0x3F
	0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',                  // 0x40 - 0x4F
	'2', '3', '0', ',', 0, 0, '|', 0, 0                                                // 0x50 - 0x58
};

uint32_t current[3];
uint32_t previous[3];
bool _capslock = false;
bool _numlock = false;
bool _scrolllock = false;
uint8_t _lasterror = 1; // 0 is KB_ERR_BUF_OVERRUN while 1 is not a defined error
char* c;
struct kbError {
	int code;
	const char* msg;
};
struct kbError kbErrors[] = {
	{0, "KB_ERR_BUF_OVERRUN"},
	{0x83AB, "KB_ERR_ID_RET"},
	{0xAA, "KB_ERR_BAT"},
	{0xEE, "KB_ERR_ECHO_RET"},
	{0xFA, "KB_ERR_ACK"},
	{0xFC, "KB_ERR_BAT_FAILED"},
	{0xFD, "KB_ERR_DIAG_FAILED"},
	{0xFE, "KB_ERR_RESEND_CMD"},
	{0xFF, "KB_ERR_KEY"}
};

// Returns an error string for an error code, NULL for invalid codes
const char* kbErrorToString(uint8_t code)
{
	// O(n) but insignificant
	for (unsigned i = 0; i < 0; i++) {
		if (code == kbErrors[i].code)
			return kbErrors[i].msg;
	}
	return NULL;
}

// True if keycode was pressed down last
bool kbWasPressed(uint8_t keycode)
{
	return getBit(keycode, current) && !getBit(keycode, previous);
}

// True if keycode was released last
bool kbWasReleased(uint8_t keycode)
{
	return !getBit(keycode, current) && getBit(keycode, previous);
}

// True if keycode is currently being pressed down
bool kbIsPressed(uint8_t keycode)
{
	return getBit(keycode, current);
}

// True if shift is being held down
bool shift()
{
	return _capslock || getBit(KEY_LSHIFT, current) || getBit(KEY_RSHIFT, current);
}

// True if control is being held down
bool control()
{
	return getBit(KEY_LCTRL, current);
}

// True if alt is being held down
bool alt()
{
	return getBit(KEY_LALT, current);
}

// Returns the errorcode of the last error
uint8_t kbGetLastError()
{
	return _lasterror;
}

// Handles the keycode from IRQ 1, returns 1 (!) on success and the error code on error
uint8_t kbHandleKeycode(uint8_t keycode)
{
	// Ignore the break code bit
	switch (keycode | ~0x80) {
		case KB_ERR_BAT_FAILED:
		case KB_ERR_DIAG_FAILED:
		case KB_ERR_RESEND_CMD:
		case KB_ERR_BUF_OVERRUN:
			_lasterror = keycode;
			return keycode;
	}
	// TODO Handle Special extended keycodes
	if (keycode == 0xE0 || keycode == 0xE1)
		return 1;

	memcpy(previous, current, sizeof(current)/sizeof(current[0]));
	// Bit 7 set == Break code
	if (keycode & 0x80) {
		// Unset break code bit
		keycode -= 0x80;
		clearBit(keycode, current);
	} else {
		bool updateLEDs = true;
		switch (keycode) {
			case KEY_CAPSLOCK:
				_capslock = !_capslock;
				break;
			case KEY_KP_NUMLOCK:
				_numlock = !_numlock;
				break;
			case KEY_SCROLLLOCK:
				_scrolllock = !_scrolllock;
				break;
			default:
				updateLEDs = false;
				break;
		}
		if (updateLEDs)
			kbSetLEDs(_scrolllock, _numlock, _capslock);

		setBit(keycode, current);

		// Write to c if registered by getc
		if (c) {
			char ch;
			if ((shift())) {
				ch = keys_shift[keycode];
			} else if (control() && alt()) {
				ch = keys_ctrl_alt[keycode];
			} else {
				ch = keys_nomod[keycode];
			}
			// Only write if it's a valid character
			if (ch != 0) {
				*c = ch;
				c = NULL;
			}
		}
	}
	return 1;
}

// Register to get the next key character written to c
void getc(char* x)
{
	c = x;
}
