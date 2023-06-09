#ifndef _SHELL_H
#define _SHELL_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <shell/cwdutils.h>

#include <stdnoreturn.h>
#include <stdbool.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define SHELL_MAX_INPUT_BUFFER 100

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct shell_state
{
	bool is_quotation_mark;
	bool is_escaped;
} shell_state_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_init(void);
noreturn void shell_start(void);

void shell_handle_output(char c);
#endif
