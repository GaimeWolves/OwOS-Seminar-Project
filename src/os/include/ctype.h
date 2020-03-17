#ifndef _CTYPE_H
#define _CTYPE_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define CT_UPPER 0b00000001 // Upper case
#define CT_LOWER 0b00000010 // Lower case
#define CT_DIGIT 0b00000100 // Digit
#define CT_CNTRL 0b00001000 // Control character
#define CT_PUNCT 0b00010000 // Punctuation
#define CT_WHITE 0b00100000 // Whitespace
#define CT_HEXDG 0b01000000 // Hex digit
#define CT_SPACE 0b10000000 // Space

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

// Lookup table holding the flags for each char
extern uint8_t __ctype_lookup[];

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

#define isalnum(c)	(__ctype_lookup[(uint8_t)c] & (CT_UPPER | CT_LOWER | CT_DIGIT))
#define isalpha(c)	(__ctype_lookup[(uint8_t)c] & (CT_UPPER | CT_LOWER))
#define iscntrl(c)	(__ctype_lookup[(uint8_t)c] & (CT_CNTRL))
#define isgraph(c)	(__ctype_lookup[(uint8_t)c] & (CT_PUNCT | CT_UPPER | CT_LOWER | CT_DIGIT))
#define islower(c)	(__ctype_lookup[(uint8_t)c] & (CT_LOWER))
#define isupper(c)	(__ctype_lookup[(uint8_t)c] & (CT_UPPER))
#define isprint(c)	(__ctype_lookup[(uint8_t)c] & (CT_PUNCT | CT_UPPER | CT_LOWER | CT_DIGIT | CT_SPACE))
#define ispunct(c)	(__ctype_lookup[(uint8_t)c] & (CT_PUNCT))
#define isspace(c)	(__ctype_lookup[(uint8_t)c] & (CT_WHITE))
#define isdigit(c)	(__ctype_lookup[(uint8_t)c] & (CT_DIGIT))
#define isxdigit(c)	(__ctype_lookup[(uint8_t)c] & (CT_DIGIT | CT_HEXDG))

#define isascii(c)	((unsigned)(c) <= 0x7F)
#define toascii(c)	((unsigned)(c) & 0x7F)

// These functions need to use the variable c more
// than once, which can lead to problems if they
// are defined as a macro
// eg. macro(c++) => c++ gets executed multiple times
int toupper(int c);
int tolower(int c);

#endif //_CTYPE_H
