#include "frame.h"

#include <display/cells.h>

#include <shell/cwdutils.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	size_t clrscr_count;
	uint8_t column;
	uint8_t row;
} shell_graphics_state_t;
//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
static shell_graphics_state_t state;

static pixel_t shell_screen_buffer[VID_MEM_SIZE];

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
	return shell_frame_get_max_row() == state.row + 1;
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
		!shell_frame_is_enough_column_space(character_count % shell_frame_get_max_column())	//+1 if current row is to full
		+ (character_count / shell_frame_get_max_column())									//+(column count occupied by the characters themself)
	);
}

void shell_frame_update_state(size_t strlen)
{
	state.column += strlen % shell_frame_get_max_column();
	state.column %= shell_frame_get_max_column();
	state.row += strlen / shell_frame_get_max_column();
	
	if(state.column < strlen % shell_frame_get_max_column())
		state.row++;
}

void shell_frame_scroll(uint8_t count)
{
	if(count >= state.row || !count)
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
void shell_frame_backspace(void)
{
	uint8_t column = state.column == 0 ? shell_frame_get_max_column() - 1 : state.column - 1;
	uint8_t row = state.column == 0 ? state.row - 1 : state.row;
	addchr(column, row, 0);
	move(column, row);

	state.column = column;
	state.row = row;

	refresh();
}

void shell_frame_goto_next_row(bool cursor)
{
	if(shell_frame_is_last_row())
		shell_frame_scroll(1);
	
	state.row++;
	state.column = 0;

	if(cursor)
	{
		move(state.column, state.row);
		refresh();
	}
}

char shell_line_string[PATH_MAX_LENGTH + 4];
char* shell_frame_get_shell_line_string(void)
{
	size_t cwdlen = strlen(cwd);
	memcpy(shell_line_string, cwd, cwdlen);
	shell_line_string[cwdlen + 0] = ' ';
	shell_line_string[cwdlen + 1] = '>';
	shell_line_string[cwdlen + 2] = ' ';
	shell_line_string[cwdlen + 3] = 0;
	return shell_line_string;
}
//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void shell_frame_init(void)
{
	clrscr();
}

void shell_frame_print_shell_line(void)
{
	if(state.column)
		shell_frame_goto_next_row(false);
	shell_frame_print_string(shell_frame_get_shell_line_string(), true);
}

void shell_frame_handle_input(char c)
{
	switch (c)
	{
	case '\n':
		shell_frame_goto_next_row(true);
		break;
	
	default:
		shell_frame_print_char(c, true);
		break;
	}
}

void shell_frame_handle_backspace(void)
{
	shell_frame_backspace();
}

void shell_frame_save_screen_state(void)
{
	copyFromBuffer(shell_screen_buffer, VID_MEM_SIZE);
	state.clrscr_count = clrscr_count;
}

void shell_frame_restore_screen_state(void)
{
	copyToBuffer(shell_screen_buffer, VID_MEM_SIZE);
}

bool shell_frame_screen_state_changed(void)
{
	return state.clrscr_count != clrscr_count;
}