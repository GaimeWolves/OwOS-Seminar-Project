#include <ctype.h>

int tolower(int c)
{
	if (__ctype_lookup[c] & CT_UPPER)
		return c + ('a' - 'A');
	else
		return c;
}

int toupper(int c)
{
	if (__ctype_lookup[c] & CT_LOWER)
		return c - ('a' - 'A');
	else
		return c;
}

uint8_t __ctype_lookup[256] =
{
	CT_CNTRL,				// 00 (NUL)
	CT_CNTRL,				// 01 (SOH)
	CT_CNTRL,				// 02 (STX)
	CT_CNTRL,				// 03 (ETX)
	CT_CNTRL,				// 04 (EOT)
	CT_CNTRL,				// 05 (ENQ)
	CT_CNTRL,				// 06 (ACK)
	CT_CNTRL,				// 07 (BEL)
	CT_CNTRL,				// 08 (BS)
	CT_WHITE + CT_CNTRL,	// 09 (HT)
	CT_WHITE + CT_CNTRL,	// 0A (LF)
	CT_WHITE + CT_CNTRL,	// 0B (VT)
	CT_WHITE + CT_CNTRL,	// 0C (FF)
	CT_WHITE + CT_CNTRL,	// 0D (CR)
	CT_CNTRL,				// 0E (SI)
	CT_CNTRL,				// 0F (SO)
	CT_CNTRL,				// 10 (DLE)
	CT_CNTRL,				// 11 (DC1)
	CT_CNTRL,				// 12 (DC2)
	CT_CNTRL,				// 13 (DC3)
	CT_CNTRL,				// 14 (DC4)
	CT_CNTRL,				// 15 (NAK)
	CT_CNTRL,				// 16 (SYN)
	CT_CNTRL,				// 17 (ETB)
	CT_CNTRL,				// 18 (CAN)
	CT_CNTRL,				// 19 (EM)
	CT_CNTRL,				// 1A (SUB)
	CT_CNTRL,				// 1B (ESC)
	CT_CNTRL,				// 1C (FS)
	CT_CNTRL,				// 1D (GS)
	CT_CNTRL,				// 1E (RS)
	CT_CNTRL,				// 1F (US)
	CT_SPACE + CT_WHITE,	// 20 Space
	CT_PUNCT,				// 21 !
	CT_PUNCT,				// 22 "
	CT_PUNCT,				// 23 #
	CT_PUNCT,				// 24 $
	CT_PUNCT,				// 25 %
	CT_PUNCT,				// 26 &
	CT_PUNCT,				// 27 '
	CT_PUNCT,				// 28 (
	CT_PUNCT,				// 29 )
	CT_PUNCT,				// 2A *
	CT_PUNCT,				// 2B +
	CT_PUNCT,				// 2C ,
	CT_PUNCT,				// 2D -
	CT_PUNCT,				// 2E .
	CT_PUNCT,				// 2F /
	CT_DIGIT + CT_HEXDG,	// 30 0
	CT_DIGIT + CT_HEXDG,	// 31 1
	CT_DIGIT + CT_HEXDG,	// 32 2
	CT_DIGIT + CT_HEXDG,	// 33 3
	CT_DIGIT + CT_HEXDG,	// 34 4
	CT_DIGIT + CT_HEXDG,	// 35 5
	CT_DIGIT + CT_HEXDG,	// 36 6
	CT_DIGIT + CT_HEXDG,	// 37 7
	CT_DIGIT + CT_HEXDG,	// 38 8
	CT_DIGIT + CT_HEXDG,	// 39 9
	CT_PUNCT,				// 3A :
	CT_PUNCT,				// 3B ;
	CT_PUNCT,				// 3C <
	CT_PUNCT,				// 3D =
	CT_PUNCT,				// 3E >
	CT_PUNCT,				// 3F ?
	CT_PUNCT,				// 40 @
	CT_UPPER + CT_HEXDG,	// 41 A
	CT_UPPER + CT_HEXDG,	// 42 B
	CT_UPPER + CT_HEXDG,	// 43 C
	CT_UPPER + CT_HEXDG,	// 44 D
	CT_UPPER + CT_HEXDG,	// 45 E
	CT_UPPER + CT_HEXDG,	// 46 F
	CT_UPPER,				// 47 G
	CT_UPPER,				// 48 H
	CT_UPPER,				// 49 I
	CT_UPPER,				// 4A J
	CT_UPPER,				// 4B K
	CT_UPPER,				// 4C L
	CT_UPPER,				// 4D M
	CT_UPPER,				// 4E N
	CT_UPPER,				// 4F O
	CT_UPPER,				// 50 P
	CT_UPPER,				// 51 Q
	CT_UPPER,				// 52 R
	CT_UPPER,				// 53 S
	CT_UPPER,				// 54 T
	CT_UPPER,				// 55 U
	CT_UPPER,				// 56 V
	CT_UPPER,				// 57 W
	CT_UPPER,				// 58 X
	CT_UPPER,				// 59 Y
	CT_UPPER,				// 5A Z
	CT_PUNCT,				// 5B [
	CT_PUNCT,				// 5C (\)
	CT_PUNCT,				// 5D ]
	CT_PUNCT,				// 5E ^
	CT_PUNCT,				// 5F _
	CT_PUNCT,				// 60 `
	CT_LOWER + CT_HEXDG,	// 61 a
	CT_LOWER + CT_HEXDG,	// 62 b
	CT_LOWER + CT_HEXDG,	// 63 c
	CT_LOWER + CT_HEXDG,	// 64 d
	CT_LOWER + CT_HEXDG,	// 65 e
	CT_LOWER + CT_HEXDG,	// 66 f
	CT_LOWER,				// 67 g
	CT_LOWER,				// 68 h
	CT_LOWER,				// 69 i
	CT_LOWER,				// 6A j
	CT_LOWER,				// 6B k
	CT_LOWER,				// 6C l
	CT_LOWER,				// 6D m
	CT_LOWER,				// 6E n
	CT_LOWER,				// 6F o
	CT_LOWER,				// 70 p
	CT_LOWER,				// 71 q
	CT_LOWER,				// 72 r
	CT_LOWER,				// 73 s
	CT_LOWER,				// 74 t
	CT_LOWER,				// 75 u
	CT_LOWER,				// 76 v
	CT_LOWER,				// 77 w
	CT_LOWER,				// 78 x
	CT_LOWER,				// 79 y
	CT_LOWER,				// 7A z
	CT_PUNCT,				// 7B {
	CT_PUNCT,				// 7C |
	CT_PUNCT,				// 7D }
	CT_PUNCT,				// 7E ~
	CT_CNTRL,				// 7F (DEL)
	// Der Rest ist 0
};
