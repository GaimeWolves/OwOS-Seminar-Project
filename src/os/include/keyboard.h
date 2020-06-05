#ifndef _KEYBOARD_H
#define _KEYBOARD_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <stream/stream.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define KEY_ESCAPE        0x01
#define KEY_1             0x02
#define KEY_2             0x03
#define KEY_3             0x04
#define KEY_4             0x05
#define KEY_5             0x06
#define KEY_6             0x07
#define KEY_7             0x08
#define KEY_8             0x09
#define KEY_9             0x0a
#define KEY_0             0x0b
#define KEY_SS            0x0c
#define KEY_HCOMMA        0x0d
#define KEY_BACKSPACE     0x0e
#define KEY_TAB           0x0f
#define KEY_Q             0x10
#define KEY_W             0x11
#define KEY_E             0x12
#define KEY_R             0x13
#define KEY_T             0x14
#define KEY_Z             0x15
#define KEY_U             0x16
#define KEY_I             0x17
#define KEY_O             0x18
#define KEY_P             0x19
#define KEY_UE            0x1a
#define KEY_PLUS          0x1b
#define KEY_RETURN        0x1c
#define KEY_LCTRL         0x1d
#define KEY_A             0x1e
#define KEY_S             0x1f
#define KEY_D             0x20
#define KEY_F             0x21
#define KEY_G             0x22
#define KEY_H             0x23
#define KEY_J             0x24
#define KEY_K             0x25
#define KEY_L             0x26
#define KEY_OE            0x27
#define KEY_AE            0x28
#define KEY_CARAT         0x29
#define KEY_LSHIFT        0x2a
#define KEY_HASH          0x2b
#define KEY_Y             0x2c
#define KEY_X             0x2d
#define KEY_C             0x2e
#define KEY_V             0x2f
#define KEY_B             0x30
#define KEY_N             0x31
#define KEY_M             0x32
#define KEY_COMMA         0x33
#define KEY_DOT           0x34
#define KEY_MINUS         0x35
#define KEY_RSHIFT        0x36
#define KEY_KP_ASTERISK   0x37
#define KEY_LALT          0x38
#define KEY_SPACE         0x39
#define KEY_CAPSLOCK      0x3a
#define KEY_F1            0x3b
#define KEY_F2            0x3c
#define KEY_F3            0x3d
#define KEY_F4            0x3e
#define KEY_F5            0x3f
#define KEY_F6            0x40
#define KEY_F7            0x41
#define KEY_F8            0x42
#define KEY_F9            0x43
#define KEY_F10           0x44
#define KEY_KP_NUMLOCK    0x45
#define KEY_SCROLLLOCK    0x46
#define KEY_HOME          0x47
#define KEY_KP_8          0x48             //keypad up     arrow
#define KEY_PAGEUP        0x49
#define KEY_KP_2          0x50             //keypad down   arrow
#define KEY_KP_3          0x51             //keypad page   down
#define KEY_KP_0          0x52             //keypad insert key
#define KEY_KP_DECIMAL    0x53             //keypad delete key
#define KEY_F11           0x57
#define KEY_F12           0x58

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initKeyboard();

void kbSetCharacterStream(characterStream_t* x);
void kbClearCharacterStream(void);
bool kbWasPressed(uint8_t keycode);
bool kbWasReleased(uint8_t keycode);
bool kbIsPressed(uint8_t keycode);
uint8_t kbGetLastError();
const char* kbErrorToString(uint8_t code);

#endif //!_KEYBOARD_H