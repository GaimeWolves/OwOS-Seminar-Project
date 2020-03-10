#ifndef _CTYPE_H
#define _CTYPE_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define CT_UPPER		0b00000001 // Großbuchstabe
#define CT_LOWER		0b00000010 // Kleinbuchstabe
#define CT_DIGIT		0b00000100 // Zahl
#define CT_CNTRL		0b00001000 // Kontrollzeichen
#define CT_PUNCT		0b00010000 // Zeichensetzung
#define CT_WHITE		0b00100000 // Leerraum
#define CT_HEXDG		0b01000000 // Hexadezimal
#define CT_SPACE		0b10000000 // Leertaste

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

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

// Diese Methoden müssen c mehr als einmal benutzen
// weswegen Makrodefinitionen Bugs verursachen würden
int toupper(int c);
int tolower(int c);

#endif //_CTYPE_H
