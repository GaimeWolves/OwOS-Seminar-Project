#ifndef _HAL_KEYBOARD_H
#define _HAL_KEYBOARD_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define KB_INTERRUPT_ADDRESS			33

#define KB_CTRL_STATS_MASK_OUT_BUF      0b00000001
#define KB_CTRL_STATS_MASK_IN_BUF       0b00000010
#define KB_CTRL_STATS_MASK_SYSTEM       0b00000100
#define KB_CTRL_STATS_MASK_CMD_DATA     0b00001000
#define KB_CTRL_STATS_MASK_LOCKED       0b00010000
#define KB_CTRL_STATS_MASK_AUX_BUF      0b00100000
#define KB_CTRL_STATS_MASK_TIMEOUT      0b01000000
#define KB_CTRL_STATS_MASK_PARITY       0b10000000
#define KB_ENC_INPUT_BUF                0x60
#define KB_ENC_CMD_REG                  0x60

#define KB_CTRL_STATS_REG               0x64
#define KB_CTRL_CMD_REG                 0x64

#define KB_ENC_CMD_SET_LED              0xED
#define KB_ENC_CMD_ECHO                 0xEE
#define KB_ENC_CMD_SCAN_CODE_SET        0xF0
#define KB_ENC_CMD_ID                   0xF2
#define KB_ENC_CMD_AUTODELAY            0xF3
#define KB_ENC_CMD_ENABLE               0xF4
#define KB_ENC_CMD_RESETWAI             0xF5
#define KB_ENC_CMD_RESETSCAN            0xF6
#define KB_ENC_CMD_ALL_AUTO             0xF7
#define KB_ENC_CMD_ALL_MAKEBREAK        0xF8
#define KB_ENC_CMD_ALL_MAKEONLY         0xF9
#define KB_ENC_CMD_ALL_MAKEBREAK_AUTO   0xFA
#define KB_ENC_CMD_SINGLE_AUTOREPEAT    0xFB
#define KB_ENC_CMD_SINGLE_MAKEBREAK     0xFC
#define KB_ENC_CMD_SINGLE_BREAKONLY     0xFD
#define KB_ENC_CMD_RESEND               0xFE
#define KB_ENC_CMD_RESET                0xFF

#define KB_CTRL_CMD_READ                0x20
#define KB_CTRL_CMD_WRITE               0x60
#define KB_CTRL_CMD_SELF_TEST           0xAA
#define KB_CTRL_CMD_INTERFACE_TEST      0xAB
#define KB_CTRL_CMD_DISABLE             0xAD
#define KB_CTRL_CMD_ENABLE              0xAE
#define KB_CTRL_CMD_READ_IN_PORT        0xC0
#define KB_CTRL_CMD_READ_OUT_PORT       0xD0
#define KB_CTRL_CMD_WRITE_OUT_PORT      0xD1
#define KB_CTRL_CMD_READ_TEST_INPUTS    0xE0
#define KB_CTRL_CMD_SYSTEM_RESET        0xFE
#define KB_CTRL_CMD_MOUSE_DISABLE       0xA7
#define KB_CTRL_CMD_MOUSE_ENABLE        0xA8
#define KB_CTRL_CMD_MOUSE_PORT_TEST     0xA9
#define KB_CTRL_CMD_MOUSE_WRITE         0xD4

#define KB_ERR_BUF_OVERRUN              0
#define KB_ERR_ID_RET                   0x83AB
#define KB_ERR_BAT                      0xAA    // Can also be LShift make code
#define KB_ERR_ECHO_RET                 0xEE
#define KB_ERR_ACK                      0xFA
#define KB_ERR_BAT_FAILED               0xFC
#define KB_ERR_DIAG_FAILED              0xFD
#define KB_ERR_RESEND_CMD               0xFE
#define KB_ERR_KEY                      0xFF

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef int (*keyboardHandler_t)(uint8_t);

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------
extern keyboardHandler_t kbKeycodeHandler;

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initKeyboardHal();
void setKeyboardHandle(keyboardHandler_t);

uint8_t kbCtrlReadStatus();
bool kbCtrlOutBufFull();
bool kbCtrlOutBufEmpty();
void kbCtrlOutBufWaitEmpty();
void kbCtrlOutBufWaitFull();
bool kbCtrlInBufFull();
bool kbCtrlInBufEmpty();
void kbCtrlInBufWaitEmpty();
void kbCtrlInBufWaitFull();
void kbCtrlSendCmd(uint8_t cmd);
void kbEnable();
void kbDisable();
uint8_t kbEncReadBuf();
bool kbSelfTest();
void kbEncSendCmd(uint8_t cmd);
void kbSetLEDs(bool scrolllock, bool numlock, bool caps);

#endif // _HAL_KEYBOARD_H