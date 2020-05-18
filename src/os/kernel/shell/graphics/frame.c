#include "frame.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	uint8_t column;
	uint8_t row;
} shell_graphics_state_t;
//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
shell_graphics_state_t state;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
uint8_t shell_frame_get_max_column(void)
{
	return MAX_COLS;
}
uint8_t shell_frame_get_max_row(void)
{
	return MAX_ROWS;
}

bool shell_frame_is_last_row(void)
{
	return shell_frame_get_max_row() == state.row;
}

bool shell_frame_is_enough_column_space(uint8_t count)
{
	return state.column + count < shell_frame_get_max_column();
}
bool shell_frame_is_enough_row_space(uint8_t count)
{
	return state.row + count < shell_frame_get_max_row();
}
bool shell_frame_is_enough_space(size_t character_count)
{
	return shell_frame_is_enough_row_space(
		shell_frame_is_enough_column_space(character_count % shell_frame_get_max_column())	//+1 if current row is to full
		+ (character_count / shell_frame_get_max_column())									//+(column count occupied by the characters themself)
	);
}

void shell_frame_update_state(size_t strlen)
{
	state.column += strlen % shell_frame_get_max_column();
	state.row += strlen / shell_frame_get_max_column();
	
	if(state.column < strlen % shell_frame_get_max_column())
		state.row++;
}

void shell_frame_scroll(uint8_t count)
{
	if(count >= state.row && !count)
		return;
	
	//scroll screen
	scrollBuffer(1);

	//scroll cursor
	setCursorContinuousPos(getCursorContinuousPos() - shell_frame_get_max_column());
	
	//print sreen
	refresh();

	state.row -= count;
}
void shell_frame_print_string(char* string, bool cursor)
{
	size_t len = strlen(string);
	if(!len)
		return;

	while(!shell_frame_is_enough_space(len))
		shell_frame_scroll(1);

	if(cursor)
		mvaddstr(state.column, state.row, string);
	else
		addstr(state.column, state.row, string);
	refresh();

	shell_frame_update_state(len);
}
void shell_frame_print_char(char c, bool cursor)
{
	while(!shell_frame_is_enough_space(1))
		shell_frame_scroll(1);

	if(cursor)
		mvaddchr(state.column, state.row, c);
	else
		addchr(state.column, state.row, c);
	refresh();

	shell_frame_update_state(1);
}

void shell_frame_goto_next_row(void)
{
	if(shell_frame_is_last_row())
		shell_frame_scroll(1);
	
	state.row++;
	state.column = 0;
}
char* shell_frame_get_shell_line_string(void)
{
	return "OwOS_Shell_0.0.1> ";
}
//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_frame_init(void)
{
	clrscr();
	state.row = (uint8_t)-1;
	shell_frame_print_shell_line();
}

void shell_frame_print_shell_line(void)
{
	shell_frame_goto_next_row();
	shell_frame_print_string(shell_frame_get_shell_line_string(), true);
}

void shell_frame_handle_input(char c)
{
	switch (c)
	{
	case '\n':
		shell_frame_goto_next_row();
		break;
	
	default:
		shell_frame_print_char(c, true);
		break;
	}
}