#include "include/stdio.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/vfs/vfs.h"
#include "../include/memory/heap.h"
#include "../include/shell/cwdutils.h"

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define SCANF_MAX_DIGITS 25 // Octal takes up at most 22 characters (plus sign, prepended zero and null terminator)
#define isodigit(c) (isdigit(c) && c < '8') // Octal equivalent of (isdigit and isxdigit)
#define ARG(type) (type)va_arg(conversion->ap, type) // Shorthand for va_arg

// Flags used in printf
#define PRINTF_FLAG_LADJUST   0x01  // Left adjust
#define PRINTF_FLAG_SIGN      0x02  // Always print sign
#define PRINTF_FLAG_SPACE     0x04  // Print space on positive value
#define PRINTF_FLAG_ALT       0x08  // Alternative representation
#define PRINTF_FLAG_ZERO      0x10  // Adjust with zeroes
#define PRINTF_FLAG_PRECISION 0x40  // Precision explicitly specified
#define PRINTF_FLAG_PARSING   0x80  // Used in conversion

// Default conversion->length: int
#define CONVERSION_LENGTH_NONE 0x00  // int
#define CONVERSION_LENGTH_HH   0x01  // char
#define CONVERSION_LENGTH_H    0x02  // short
#define CONVERSION_LENGTH_L    0x04  // long
#define CONVERSION_LENGTH_LL   0x08  // long long
#define CONVERSION_LENGTH_J    0x10  // intmax_t
#define CONVERSION_LENGTH_Z    0x20  // size_t
#define CONVERSION_LENGTH_T    0x40  // ptrdiff_t

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

struct printf_conv_t;
struct scanf_conv_t;

typedef void (*printf_putc_callback)(const char ch, struct printf_conv_t *conv);
typedef void (*printf_puts_callback)(const char *str, size_t size, struct printf_conv_t *conv);

typedef int (*scanf_getc_callback)(struct scanf_conv_t *conv);
typedef void (*scanf_ungetc_callback)(struct scanf_conv_t *conv, const char ch);

// Holds the state of the current printf-conversion->for use in helper functions
typedef struct printf_conv_t
{
	// State of current conversion
	uint8_t flags;
	int minimal_width;
	int precision;
	uint8_t length;
	char specifier;

	// State of current printf call
	const char *format;
	uintptr_t buffer; // May be char*, stdin*/stdout* or FILE*
	size_t bufsz;
	int written;
	va_list ap;

	// Callbacks because writing to char*, characterStream* and FILE* is different
	printf_putc_callback putc;
	printf_puts_callback puts;
} printf_conv_t;

// Holds the state of teh current scanf-conversion
typedef struct scanf_conv_t
{
	bool suppress;     // Suppress assignment into variable
	int maximum_width;
	uint8_t length;
	char specifier;

	const char *format;
	uintptr_t buffer; // May be char*, stdin*/stdout* or FILE*
	int assigned;
	int read;
	va_list ap;

	scanf_getc_callback getc;
	scanf_ungetc_callback ungetc;
} scanf_conv_t;

// Union so that one method can assign unsigned and signed numbers
typedef union scanf_number_t
{
	unsigned long long unsigned_num;
	long long signed_num;
} scanf_number_t;

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

// Three versions of every callback for char**, characterStream* and FILE*
static void str_putc(const char ch, printf_conv_t *conv);
static void file_putc(const char ch, printf_conv_t *conv);

static void str_puts(const char *str, size_t size, printf_conv_t *conv);
static void file_puts(const char *str, size_t size, printf_conv_t *conv);

static int str_getc(scanf_conv_t *conv);
static int file_getc(scanf_conv_t *conv);

static void str_ungetc(scanf_conv_t *conv, const char ch);
static void file_ungetc(scanf_conv_t *conv, const char ch);


// "Global" helper methods used in scanf and printf
static void parse_length(const char **format, uint8_t *length);

// Scanf helper methods
static void scanf_assign_number(scanf_conv_t *conversion, scanf_number_t num, bool sign);

// Printf helper methods
static void write_padding(printf_conv_t *conversion, bool ladjust, char fill);
static void write_string(printf_conv_t *conversion, const char *s, size_t size);
static void write_char(printf_conv_t *conversion, const char c);
static void write_char_seq(printf_conv_t *conversion, const char c, size_t count);

static void printf_parse_number(printf_conv_t *conversion, char *buffer, void* var);
static void printf_parse_varg(printf_conv_t *conversion, char *buffer);
static void printf_parse_flags(printf_conv_t *conversion);
static void printf_parse_width(printf_conv_t *conversion);
static char printf_parse_precision(printf_conv_t *conversion);

// Conversion struct prepared by specific function and passed to generic printf/scanf
static void generic_printf(printf_conv_t *conversion);
static void generic_scanf(scanf_conv_t * conversion);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

static void str_putc(const char ch, printf_conv_t *conv)
{
	char **buffer = (char**)&conv->buffer;
	*(*buffer)++ = ch;
}

static void file_putc(const char ch, printf_conv_t *conv)
{
	FILE *file = (FILE*)conv->buffer;
	fputc(ch, file);
}

static void str_puts(const char *str, size_t size, printf_conv_t *conv)
{
	char **buffer = (char**)&conv->buffer;
	strncpy(*buffer, str, size);
	*buffer += size;
}

static void file_puts(const char *str, size_t size, printf_conv_t *conv)
{
	FILE *file = (FILE*)conv->buffer;

	size++;
	while(size-- > 0)
		fputc(*str++, file);
}

static int str_getc(scanf_conv_t *conv)
{
	char **buffer = (char**)&conv->buffer;
	char got = *++(*buffer);

	conv->read++;

	if (got == '\0')
		return EOF;
	else
		return (int)got;
}

static int file_getc(scanf_conv_t *conv)
{
	conv->read++;
	FILE *file = (FILE*)conv->buffer;
	return fgetc(file);
}

static void str_ungetc(scanf_conv_t *conv, const char ch)
{
	conv->read--;
	char **buffer = (char**)&conv->buffer;
	*--(*buffer) = ch;
}

static void file_ungetc(scanf_conv_t *conv, const char ch)
{
	conv->read--;
	FILE *file = (FILE*)conv->buffer;
	ungetc(ch, file);
}


// Helper function printing the padding for the conversion
static void write_padding(printf_conv_t *conversion, bool ladjust, char fill)
{
	if (conversion->minimal_width <= 0)
		return;

	if (ladjust != (conversion->flags & PRINTF_FLAG_LADJUST))
		return;

	write_char_seq(conversion, fill, conversion->minimal_width);
}

// Helper function to write a string to the buffer
// updating the bufsz and written variables
static void write_string(printf_conv_t *conversion, const char *s, size_t size)
{
	conversion->written += size;
	size = size > conversion->bufsz - 1 ? conversion->bufsz - 1 : size;

	conversion->puts(s, size, conversion);
	conversion->bufsz -= size;
}

// Helper function to write a char to the buffer
// updating the bufsz and written variables
static void write_char(printf_conv_t *conversion, const char c)
{
	conversion->written++;

	if (conversion->bufsz > 1)
	{
		conversion->putc(c, conversion);
		conversion->bufsz--;
	}
}

static void write_char_seq(printf_conv_t *conversion, const char c, size_t count)
{
	conversion->written += count;

	count = count > conversion->bufsz - 1 ? conversion->bufsz - 1 : count;

	while (count--)
	{
		conversion->putc(c, conversion);
		conversion->bufsz--;
	}
}

// Helper function to parse value by conversion->specifier
// Pointer gets cast to signed or unsigned type accordingly
static void printf_parse_number(printf_conv_t *conversion, char* buffer, void* var)
{
	switch(conversion->specifier)
	{
		case 'd':
		case 'i':
		{
			lltoa(*(long long*)var, buffer, 10, false);
			break;
		}
		case 'u':
		{
			ulltoa(*(unsigned long long*)var, buffer, 10, false);
			break;
		}
		case 'x':
		case 'X':
		{
			ulltoa(*(unsigned long long*)var, buffer, 16, false);
			break;
		}
		case 'o':
		{
			ulltoa(*(unsigned long long*)var, buffer, 8, false);
			break;
		}
		default:
			break;
	}

}

// Helper function to load value by length specifier
static void printf_parse_varg(printf_conv_t *conversion, char* buffer)
{
	// Max value gets cast to needed value
	uint64_t var;

	switch(conversion->length)
	{
		case CONVERSION_LENGTH_HH:
		{
			// First convert to int because of integer promotion in var args
			var = (char)ARG(int);
			break;
		}
		case CONVERSION_LENGTH_H:
		{
			// First convert to int because of integer promotion in var args
			var = (short)ARG(int);
			break;
		}
		case CONVERSION_LENGTH_NONE:
		{
			var = ARG(int);
			break;
		}
		case CONVERSION_LENGTH_L:
		{
			var = ARG(long);
			break;
		}
		case CONVERSION_LENGTH_LL:
		{
			var = ARG(unsigned long long);
			break;
		}
		case CONVERSION_LENGTH_J:
		{
			var = ARG(uintmax_t);
			break;
		}
		case CONVERSION_LENGTH_Z:
		{
			var = ARG(size_t);
			break;
		}
		case CONVERSION_LENGTH_T:
		{
			var = ARG(ptrdiff_t);
			break;
		}
		default:
			break;
	}

	printf_parse_number(conversion, buffer, &var);
}

// Helper function to parse the flags for the conversion
static void printf_parse_flags(printf_conv_t *conversion)
{
	while(conversion->format[0] && conversion->flags & PRINTF_FLAG_PARSING)
	{
		switch(conversion->format[0])
		{
			case '-':
			{
				conversion->flags |= PRINTF_FLAG_LADJUST;
				break;
			}
			case '+':
			{
				conversion->flags |= PRINTF_FLAG_SIGN;
				break;
			}
			case ' ':
			{
				conversion->flags |= PRINTF_FLAG_SPACE;
				break;
			}
			case '#':
			{
				conversion->flags |= PRINTF_FLAG_ALT;
				break;
			}
			case '0':
			{
				conversion->flags |= PRINTF_FLAG_ZERO;
				break;
			}
			default:
			{
				// Clear parsing flag
				conversion->flags ^= PRINTF_FLAG_PARSING;
				conversion->format--; // So we don't skip anything
				break;
			}
		}
		conversion->format++;
	}
}

// Helper function to parse the specified minimal width
static void printf_parse_width(printf_conv_t *conversion)
{
	if (conversion->format[0] == '*')
	{
		conversion->minimal_width = ARG(int);
		conversion->format++;
	}
	else if (isdigit(conversion->format[0]))
	{
		char* str_end;
		conversion->minimal_width = (int)strtol(conversion->format, &str_end, 10);
		conversion->format = str_end;
	}

	if (conversion->minimal_width < 0)
	{
		conversion->flags |= PRINTF_FLAG_LADJUST;
		conversion->minimal_width *= -1;
	}
}

// Helper function to parse the specified precision
// Returns the character used for padding a converted integer
static char printf_parse_precision(printf_conv_t *conversion)
{
	char fill = ' ';

	if (conversion->format[0] == '.')
	{
		conversion->format++;
		conversion->flags |= PRINTF_FLAG_PRECISION;
		
		if (conversion->format[0] == '*')
		{
			conversion->precision = ARG(int);
			conversion->format++;
		}
		else if (isdigit(conversion->format[0]))
		{
			char* str_end;
			conversion->precision = (int)strtol(conversion->format, &str_end, 10);
			conversion->format = str_end;
		}

		if (conversion->precision < 0)
			conversion->precision = 0;
	}
	else if (conversion->flags & PRINTF_FLAG_ZERO)
		fill = '0'; // Use '0' as padding only if precision not specified

	return fill;
}

// Helper function to parse the length modifier
static void parse_length(const char **format, uint8_t *length)
{
	switch((*format)[0])
	{
		case 'h':
		{
			if ((*format)[1] == 'h')
			{
				*length = CONVERSION_LENGTH_HH;
				(*format)++;
			}
			else
				*length = CONVERSION_LENGTH_H;
			break;
		}
		case 'l':
		{
			if ((*format)[1] == 'l')
			{
				*length = CONVERSION_LENGTH_LL;
				(*format)++;
			}
			else
				*length = CONVERSION_LENGTH_L;
			break;
		}
		case 'j':
		{
			*length = CONVERSION_LENGTH_J;
			break;
		}
		case 'z':
		{
			*length = CONVERSION_LENGTH_Z;
			break;
		}
		case 't':
		{
			*length = CONVERSION_LENGTH_T;
			break;
		}
		default:
		{
			(*format)--; // So we don't skip anything
			break;
		}
	}
	(*format)++;
}

// Assing the number to the argument depending on the length specifier
static void scanf_assign_number(scanf_conv_t *conversion, scanf_number_t num, bool sign)
{
	if (conversion->specifier == 'p') // Pointers assign a void**
	{
		*ARG(void**) = (void*)(unsigned long)num.unsigned_num;
		return;
	}

	switch(conversion->length)
	{
		case CONVERSION_LENGTH_HH:
		{
			if (sign)
				*ARG(char*) = num.signed_num;
			else
				*ARG(unsigned char*) = num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_H:
		{
			if (sign)
				*ARG(short*) = num.signed_num;
			else
				*ARG(unsigned short*) = num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_NONE:
		{
			if (sign)
				*ARG(int*) = num.signed_num;
			else
				*ARG(unsigned int*) = num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_L:
		{
			if (sign)
				*ARG(long*) = num.signed_num;
			else
				*ARG(unsigned long*) = num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_LL:
		{
			if (sign)
				*ARG(long long*) = num.signed_num;
			else
				*ARG(unsigned long long*) = num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_J:
		{
			if (sign)
				*ARG(intmax_t*) = num.signed_num;
			else
				*ARG(uintmax_t*) = num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_Z:
		{
			*ARG(size_t*) = sign ? (unsigned long long)num.signed_num : num.unsigned_num;
			break;
		}
		case CONVERSION_LENGTH_T:
		{
			*ARG(ptrdiff_t*) = sign ? (unsigned long long)num.signed_num : num.unsigned_num;
			break;
		}
		default:
			break;
	}

}

static void generic_scanf(scanf_conv_t *conversion)
{
	if (!conversion->buffer || !conversion->format)
	{
		conversion->assigned = -1;
		return;
	}

	const char *format = conversion->format;
	int chr = 0; // Variable for currently read char

	while(*format)
	{
		if (chr == EOF) // Format continues but stream ended
			return;

		if (*format == '%')
		{
			conversion->specifier = 0;
			conversion->maximum_width = -1;
			conversion->length = 0;
			conversion->suppress = false;

			format++; // Skip % char

			if (*format == '*') // Suppress
			{
				format++;
				conversion->suppress = true;
			}

			if (isdigit(*format)) // Maximum field width
			{
				char *str_end = NULL;
				int len = strtol(format, &str_end, 10);
				format = str_end;

				if (len < 1)
					break;

				conversion->maximum_width = len;
			}
			
			parse_length(&format, &conversion->length); // Argument length

			int base = 0;
			bool sign = false;

			conversion->specifier = *format++;
			switch(conversion->specifier)
			{
				case '%': // Match % literal
				{
					if (conversion->maximum_width || conversion->length || conversion->suppress)
						break;

					// Consume leading whitespace (same in some other conversions)
					while(isspace(chr = conversion->getc(conversion)));

					if (chr != '%') // Also works for EOF exception
						return;
					
					// Doesn't count towards assigned count so we decrement
					// as it gets automatically incremented
					conversion->assigned--;

					break;
				}
				case 'c': // Match any character(s)
				{
					char *buffer = ARG(char*);
					if (conversion->maximum_width == -1)
						conversion->maximum_width = 1;

					while(conversion->maximum_width--)
					{
						chr = conversion->getc(conversion);

						if (chr == EOF)
							return;

						*buffer++ = (char)chr;
					}

					break;
				}
				case 's': // Match string (break on whitespace)
				{
					char *buffer = ARG(char*);

					while(isspace(chr = conversion->getc(conversion)));

					do
					{
						if (chr == EOF)
							return;

						*buffer++ = (char)chr;
						chr = conversion->getc(conversion);

						if (conversion->maximum_width != -1)
						{
							if (--conversion->maximum_width == 0)
								break;
						}
					} while (!isspace(chr));

					conversion->ungetc(conversion, chr);
					*buffer = '\0';

					break;
				}
				case '[': // Match sequence of chars from the [set]
				{
					char *buffer = ARG(char*);
					bool negate = false;
					bool allowed[256] = { 0 }; // Allowed ASCII chars

					// Handle negation
					if (*format == '^')
					{
						negate = true;
						format++;
					}

					// Handle [-]abc] and []-abc] examples
					if (*format == ']' || *format == '-')
					{
						allowed[(uint8_t)*format] = true;
						format++;
					}
					
					if (*format == ']' || *format == '-')
					{
						allowed[(uint8_t)*format] = true;
						format++;
					}

					// Parse set into map
					uint8_t fc = *format;
					while((fc = *format++) != '\0' && fc != ']')
					{
						// Valid range
						if (fc == '-' && *format != '\0' && *format != ']' && (uint8_t)format[-2] <= (uint8_t)*format)
						{
							for (fc = (uint8_t)format[-2]; fc < (uint8_t)*format; ++fc)
								allowed[fc] = true;
						}
						else
							allowed[fc] = true;
					}

					if (fc == '\0') // Invalid set format
						return;

					chr = conversion->getc(conversion);
					if (chr == EOF)
						return;

					while((allowed[chr] && !negate) || (!allowed[chr] && negate))
					{
						*buffer++ = (char)chr;
						chr = conversion->getc(conversion);

						if (conversion->maximum_width != -1)
						{
							if (--conversion->maximum_width == 0)
								break;
						}
					}

					conversion->ungetc(conversion, chr);
					*buffer = '\0';

					break;
				}

				// Combine every number specifier with the use of goto
				case 'p':
				{
					conversion->length = CONVERSION_LENGTH_L; // Force 32-bit
					base = 16; // Pointers are in hex
					goto scanf_number_conv;
				}
				case 'x':
				case 'X':
				{
					base = 16;
					goto scanf_number_conv;
				}
				case 'o':
				{
					base = 8;
					goto scanf_number_conv;
				}
				case 'u':
				{
					base = 10;
					goto scanf_number_conv;
				}
				case 'd':
				{
					base = 10;
					sign = true;
					goto scanf_number_conv;
				}
				case 'i':
				{
					sign = true;
					base = 10;

				scanf_number_conv: ; // Converts the number and assigns it
					char number[SCANF_MAX_DIGITS] = { 0 };
					int index = 0;

					while(isspace(chr = conversion->getc(conversion)));

					// Check sign
					if (chr == '+' || chr == '-')
					{
						if (!sign && chr == '-') // Invalid number format
							return;

						if (conversion->maximum_width != -1)
							conversion->maximum_width--;

						number[index++] = (char)chr;
						chr = conversion->getc(conversion);
					}

					// Identify base
					if (chr == '0' && conversion->maximum_width != 0)
					{
						if (conversion->maximum_width != -1)
							conversion->maximum_width--;

						number[index++] = (char)chr;
						chr = conversion->getc(conversion);

						if (tolower(chr) == 'x' && conversion->maximum_width != 0)
						{
							if (base == 16 || base == 0)
							{
								chr = conversion->getc(conversion);
								base = 16;
							}
						}
						else if (base == 0)
							base = 8;
					}

					// Get rest of the number into the buffer
					while(isxdigit(chr) && index < SCANF_MAX_DIGITS)
					{
						if (chr == EOF)
							return;

						if (base == 8 && !isodigit(chr))
							break;
						else if (base == 10 && !isdigit(chr))
							break;

						number[index++] = (char)chr;
						chr = conversion->getc(conversion);

						if (conversion->maximum_width != -1)
						{
							if (--conversion->maximum_width == 0)
								break;
						}
					}

					conversion->ungetc(conversion, chr);

					if (number[0] == '\0') // No number read -> matching error
						return;

					scanf_number_t num;
					if (sign)
						num.signed_num = strtoll(number, NULL, base);
					else
						num.unsigned_num = strtoull(number, NULL, base);

					if (!conversion->suppress)
						scanf_assign_number(conversion, num, sign);

					break;
				}
				case 'n': // Assign number of chars read
				{
					scanf_number_t num;
					num.signed_num = conversion->read;

					if (!conversion->suppress)
						scanf_assign_number(conversion, num, true);

					// C Standard says it doesn't count towards the
					// assignment count even though it clearly
					// assignes a number
					conversion->assigned--;
					break;
				}
				default:
				{
					format--; // Treat the invalid conversion specifier as normal char
					break;
				}
			}

			conversion->assigned++; // Increment assignment counter
		}
		else if (isspace(*format)) // Cosume all whitespace characters
		{
			while(isspace(chr = conversion->getc(conversion)));
			conversion->ungetc(conversion, chr);
			format++;
		}
		else // Match literal character
		{
			int ch = conversion->getc(conversion);

			if (ch == EOF)
				return;

			if ((char)ch != *format++)
				return;
		}
	}
}

static void generic_printf(printf_conv_t *conversion)
{
	if (!conversion->buffer || !conversion->format)
	{
		conversion->written = -1;
		return;
	}

	const char **format = &conversion->format;
	size_t *bufsz = &conversion->bufsz;

	while(**format && *bufsz > 1)
	{
		if (**format == '%')
		{
			conversion->specifier = 0;
			conversion->flags = 0;
			conversion->length = 0;
			conversion->precision = 0;
			conversion->minimal_width = 0;

			(*format)++;

			// Parse optional flags
			printf_parse_flags(conversion);

			// Parse minimal width modifier
			printf_parse_width(conversion);

			// Parse precision
			// Character used for padding (only applies to integer conversion->
			char fill = printf_parse_precision(conversion);

			// Parse length modifier
			parse_length(format, &conversion->length);

			// Parse format specifier
			conversion->specifier = *(*format)++;
			switch (conversion->specifier)
			{
				case '%': // Print % symbol
				{
					// Full specifier must be %%
					if (conversion->flags | conversion->length | conversion->minimal_width | conversion->precision)
						break;

					write_char(conversion, '%');

					break;
				}
				case 'c': // Print char
				{
					conversion->written++;

					// Specification says to first convert to unsigned char
					unsigned char c = (unsigned char)ARG(int);
					
					conversion->minimal_width--;
					write_padding(conversion, false, ' ');
					
					write_char(conversion, c);

					write_padding(conversion, true, ' ');

					break;
				}
				case 's': // Print string
				{
					char* s = ARG(char*);
					size_t size = strlen(s);
					
					// Precision declares maximum characters to be printed
					if (conversion->flags & PRINTF_FLAG_PRECISION)
						size = size > (size_t)conversion->precision ? (size_t)conversion->precision : size;

					conversion->minimal_width -= size;
					write_padding(conversion, false, ' ');

					write_string(conversion, s, size);
					
					write_padding(conversion, true, ' ');

					break;
				}
				case 'd': // Print signed decimal
				case 'i':
				case 'u': // Print unsigned decimal
				{
					// Decimal representation takes up at most 21 characters
					char repr[21] = { 0 };
					printf_parse_varg(conversion, repr);
					
					size_t size = strlen(repr);

					// Sign char calculated based on sign flag and sign of converted number
					char sign = 0;
					if (conversion->specifier != 'u')
					{
						if (repr[0] == '-')
						{
							memmove((void*)repr, (void*)(repr + 1), size);
							sign = '-';
							size--;
						}
						else if (conversion->flags & PRINTF_FLAG_SIGN)
							sign = '+';
						else if (conversion->flags & PRINTF_FLAG_SPACE)
							sign = ' ';
					}
				
					// Padding calculated depending on precision and zero flag
					size_t padding = 0;
					if (~conversion->flags & PRINTF_FLAG_PRECISION && conversion->flags & PRINTF_FLAG_ZERO)
						padding = size > (size_t)conversion->minimal_width ? 0 : conversion->minimal_width - size;
					else
						padding = size > (size_t)conversion->precision ? 0 : conversion->precision - size;

					// If precision and value both equal 0 nothing is printed
					if (!(conversion->flags & PRINTF_FLAG_PRECISION) || conversion->precision > 0 || repr[0] != '0')
						conversion->minimal_width -= size + padding;

					write_padding(conversion, false, fill);

					// Only print if precision and representation not zero
					if (!(conversion->flags & PRINTF_FLAG_PRECISION) || conversion->precision > 0 || repr[0] != '0')
					{
						if (sign)
							write_char(conversion, sign);

						write_char_seq(conversion, '0', padding);
						write_string(conversion, repr, size);
					}

					write_padding(conversion, true, fill);

					break;
				}
				case 'o': // Print octal number
				{
					// Octal representation takes up at most 22 characters
					char repr[22] = { 0 };
					printf_parse_varg(conversion, repr);
					
					size_t size = strlen(repr);
					
					// Padding calculated depending on precision and zero flag
					size_t padding = 0;
					if (~conversion->flags & PRINTF_FLAG_PRECISION && conversion->flags & PRINTF_FLAG_ZERO)
						padding = size > (size_t)conversion->minimal_width ? 0 : conversion->minimal_width - size;
					else
						padding = size > (size_t)conversion->precision ? 0 : conversion->precision - size;

					// If precision and value both equal 0 nothing is printed
					if (!(conversion->flags & PRINTF_FLAG_PRECISION) || conversion->precision > 0 || repr[0] != '0')
						conversion->minimal_width -= size + padding;

					// Alternative representation reqires at least one leading zero
					if (conversion->flags & PRINTF_FLAG_ALT)
						conversion->minimal_width--;

					write_padding(conversion, false, fill);

					// Alternative representation prints a preceeding zero
					if (conversion->flags & PRINTF_FLAG_ALT)
						write_char(conversion, '0');
					
					// Only print if precision and representation not zero
					if (!(conversion->flags & PRINTF_FLAG_PRECISION) || conversion->precision > 0 || repr[0] != '0')
					{
						write_char_seq(conversion, '0', padding);
						write_string(conversion, repr, size);
					}

					write_padding(conversion, true, fill);

					break;
				}
				case 'x': // Print hexadecimal with lowercase letters
				case 'X': // with uppercase letters
				{
					// Hexadecimal representation takes up at most 17 characters
					char repr[17] = { 0 };
					printf_parse_varg(conversion, repr);
					
					size_t size = strlen(repr);

					// Upper or lower case depending in specifier;
					for (size_t i = 0; i < size; i++)
						repr[i] = islower(conversion->specifier) ? tolower(repr[i]) : repr[i];

					// Padding calculated depending on precision and zero flag
					size_t padding = 0;
					if (~conversion->flags & PRINTF_FLAG_PRECISION && conversion->flags & PRINTF_FLAG_ZERO)
						padding = size > (size_t)conversion->minimal_width ? 0 : conversion->minimal_width - size;
					else
						padding = size > (size_t)conversion->precision ? 0 : conversion->precision - size;

					if (!(conversion->flags & PRINTF_FLAG_PRECISION) || conversion->precision > 0 || repr[0] != '0')
						conversion->minimal_width -= size + padding;

					if (conversion->flags & PRINTF_FLAG_ALT && repr[0] != '0')
						conversion->minimal_width -= 2;

					write_padding(conversion, false, fill);

					// Alternative representation prints 0x before the value
					if (conversion->flags & PRINTF_FLAG_ALT && repr[0] != '0')
					{
						write_char(conversion, '0');
						write_char(conversion, conversion->specifier); // x or X depending on specifier
					}

					// If precision and value both equal 0 nothing is printed
					if (!(conversion->flags & PRINTF_FLAG_PRECISION) || conversion->precision > 0 || repr[0] != '0')
					{
						write_char_seq(conversion, '0', padding);
						write_string(conversion, repr, size);
					}

					write_padding(conversion, true, fill);

					break;
				}
				case 'n': // Store characters written
				{
					switch(conversion->length)
					{
						case CONVERSION_LENGTH_HH:
						{
							*ARG(char*) = (char)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_H:
						{
							*ARG(short*) = (short)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_NONE:
						{
							*ARG(int*) = (int)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_L:
						{
							*ARG(long*) = (long)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_LL:
						{
							*ARG(long long*) = (long long)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_J:
						{
							*ARG(intmax_t*) = (intmax_t)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_Z:
						{
							*ARG(size_t*) = (size_t)conversion->written;
							break;
						}
						case CONVERSION_LENGTH_T:
						{
							*ARG(ptrdiff_t*) = (ptrdiff_t)conversion->written;
							break;
						}
						default:
							break;
					}

					break;
				}
				case 'p': // Print pointer
				{
					unsigned long ptr = ARG(unsigned long);
					char repr[11] = { '0', 'x', 0 };
					uitoa((size_t)ptr, repr + 2, 16, true);
					
					conversion->minimal_width -= 10;
					write_padding(conversion, false, ' ');

					write_string(conversion, repr, 10);

					write_padding(conversion, true, ' ');

					break;
				}
				default:
					format--; // So we don't skip anything
					break;
			}
		}
		else
		{
			// Write character from format
			conversion->written++;
			if (*bufsz > 1)
			{
				conversion->putc(**format, conversion);
				(*bufsz)--;
			}
		}

		(*format)++;
	}

	// Terminate with null byte if sprintf was used
    if (bufsz && conversion->putc == (printf_putc_callback)str_putc)
        conversion->putc('\0', conversion);
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

FILE *fopen(const char *filename, const char *mode)
{
	if (!mode || !filename)
	{
		errno = !filename ? ENOENT : EINVAL;
		return NULL;
	}

	char* path = resolve_path(filename);

	FILE *file;
	if (!(file = vfsOpen(path ? path : filename, mode)))
		errno = ENOENT;

	free(path);

	return file;
}

FILE *freopen(const char *filename, const char *mode, FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return NULL;
	}

	if (!mode)
	{
		errno = EINVAL;
		return NULL;
	}

	char* path = resolve_path(filename);

	FILE *file;
	if (!(file = vfsReopen(path ? path : filename, mode, stream)))
		errno = ENOENT;

	free(path);

	return file;
}

int fclose(FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	vfsClose(stream);
	return 0;
}

int fflush(FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	if (vfsFlush(stream))
	{
		errno = ENOSPC;
		return EOF;
	}

	return 0;
}

void setbuf(FILE *stream, char *buffer)
{
	if (buffer)
		vfsSetvbuf(stream, buffer, _IOFBF, BUFSIZ);
	else
		vfsSetvbuf(stream, NULL, _IONBF, 0); // Turn off buffering
}

int setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	return vfsSetvbuf(stream, buffer, mode, size);
}

size_t fread(void *buffer, size_t size, size_t count, FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return 0;
	}

	size_t read = vfsRead(stream, buffer, size * count);
	return read / size;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return 0;
	}

	size_t written = vfsWrite(stream, buffer, size * count);
	return written / size;
}

int fgetc(FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	return vfsGetc(stream);
}

int getc(FILE *stream)
{
	return fgetc(stream);
}

char *fgets(char *str, int count, FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return NULL;
	}

	if (!str || count < 1)
	{
		errno = EINVAL;
		return NULL;
	}

	return vfsGets(str, count, stream);
}

int fputc(int ch, FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	return vfsPutc(ch, stream);
}

int putc(int ch, FILE *stream)
{
	return fputc(ch, stream);
}

int fputs(const char *str, FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	return vfsPuts(str, stream);
}

int getchar()
{
	return vfsGetc(stdin);
}

char *gets(char *str)
{
	if (!str)
		return NULL;

	char chr = vfsGetc(stdin);

	while (chr != '\n')
	{
		*str++ = chr;
		chr = vfsGetc(stdin);
	}

	*str = '\0';

	return str;
}

char *gets_s(char *str, size_t n)
{
	if (!str || n == 0)
		return NULL;

	while(n-- > 1)
	{
		char chr = vfsGetc(stdin);

		if (chr == '\n')
			break;

		*str++ = chr;
	}

	*str = '\0';

	return str;
}

int putchar(int ch)
{
	vfsPutc((char)ch, stdout);
	return 0;
}

int puts(const char *str)
{
	vfsPuts(str, stdout);
	vfsPutc('\n', stdout);

	return 0;
}

int ungetc(int ch, FILE *stream)
{
	return vfsUngetc(ch, stream);
}

int scanf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int assigned = vscanf(format, ap);

	va_end(ap);

	return assigned;
}

int fscanf(FILE *stream, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int assigned = vfscanf(stream, format, ap);

	va_end(ap);

	return assigned;
}

int sscanf(const char *buffer, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	// To prevent code duplication the maximum return value
	// for the architecture is used
	int assigned = vsscanf(buffer, format, ap);

	va_end(ap);

	return assigned;
}

int vscanf(const char *format, va_list ap)
{
	scanf_conv_t conversion = { 0 };

	conversion.ap = ap;
	conversion.buffer = (uintptr_t)stdin;
	conversion.getc = (scanf_getc_callback)file_getc;
	conversion.ungetc = (scanf_ungetc_callback)file_ungetc;
	conversion.format = format;

	generic_scanf(&conversion);

	return conversion.assigned;
}

int vfscanf(FILE *stream, const char *format, va_list ap)
{
	scanf_conv_t conversion = { 0 };

	conversion.ap = ap;
	conversion.buffer = (uintptr_t)stream;
	conversion.getc = (scanf_getc_callback)file_getc;
	conversion.ungetc = (scanf_ungetc_callback)file_ungetc;
	conversion.format = format;

	generic_scanf(&conversion);

	return conversion.assigned;
}

int vsscanf(const char *buffer, const char *format, va_list ap)
{
	scanf_conv_t conversion = { 0 };

	conversion.ap = ap;
	conversion.buffer = (uintptr_t)buffer;
	conversion.getc = (scanf_getc_callback)str_getc;
	conversion.ungetc = (scanf_ungetc_callback)str_ungetc;
	conversion.format = format;

	generic_scanf(&conversion);

	return conversion.assigned;
}

int printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int written = vprintf(format, ap);

	va_end(ap);

	return written;
}

int fprintf(FILE *stream, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int written = vfprintf(stream, format, ap);

	va_end(ap);

	return written;
}

int sprintf(char *buffer, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	// To prevent code duplication the maximum return value
	// for the architecture is used
	int written = vsnprintf(buffer, INT_MAX, format, ap);

	va_end(ap);

	return written;
}

int snprintf(char *buffer, size_t bufsz, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int written = vsnprintf(buffer, bufsz, format, ap);

	va_end(ap);
	return written;
}

int vprintf(const char *format, va_list ap)
{
	printf_conv_t conversion = { 0 };

	conversion.ap = ap;
	conversion.buffer = (uintptr_t)stdout;
	conversion.bufsz = INT_MAX;
	conversion.putc = (printf_putc_callback)file_putc;
	conversion.puts = (printf_puts_callback)file_puts;
	conversion.format = format;

	generic_printf(&conversion);

	return conversion.written;
}

int vfprintf(FILE *stream, const char *format, va_list ap)
{
	printf_conv_t conversion = { 0 };

	conversion.ap = ap;
	conversion.buffer = (uintptr_t)stream;
	conversion.bufsz = INT_MAX;
	conversion.putc = (printf_putc_callback)file_putc;
	conversion.puts = (printf_puts_callback)file_puts;
	conversion.format = format;

	generic_printf(&conversion);

	return conversion.written;
}

int vsprintf(char *buffer, const char *format, va_list ap)
{
	// To prevent code duplication the maximum return value
	// for the architecture is used
	return vsnprintf(buffer, INT_MAX, format, ap);
}

int vsnprintf(char *buffer, size_t bufsz, const char *format, va_list ap)
{
	printf_conv_t conversion = { 0 };

	conversion.ap = ap;
	conversion.buffer = (uintptr_t)buffer;
	conversion.bufsz = bufsz;
	conversion.putc = (printf_putc_callback)str_putc;
	conversion.puts = (printf_puts_callback)str_puts;
	conversion.format = format;

	generic_printf(&conversion);

	return conversion.written;
}

long ftell(FILE *stream)
{
	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	if (stream->pos > LONG_MAX)
		errno = EOVERFLOW;

	return stream->pos;
}

int fgetpos(FILE *stream, fpos_t *pos)
{
	if (!stream || !pos)
	{
		errno = EBADF;
		return EOF;
	}

	*pos = stream->pos;

	return 0;
}

int fseek(FILE *stream, long offset, int origin)
{
	if (origin < SEEK_SET || origin > SEEK_END)
	{
		errno = EINVAL;
		return EOF;
	}

	if (offset < 0)
	{
		if (origin == SEEK_SET)
		{
			errno = EINVAL;
			return EOF;
		}
		else if (stream->pos < (unsigned long)-offset)
		{
			errno = EINVAL;
			return EOF;
		}
	}

	if (!stream)
	{
		errno = EBADF;
		return EOF;
	}

	return vfsSeek(stream, offset, origin);
}

int fsetpos(FILE *stream, const fpos_t *pos)
{
	if (!stream || !pos)
	{
		errno = EBADF;
		return EOF;
	}

	vfsSeek(stream, *pos, SEEK_SET);

	return 0;
}

void rewind(FILE *stream)
{
	fseek(stream, 0, SEEK_SET);
}

void clearerr(FILE *stream)
{
	if (stream)
		stream->flags &= ~(F_ERROR);
}

int feof(FILE *stream)
{
	return stream->flags & O_EOF;
}

int ferror(FILE *stream)
{
	return stream ? stream->flags & F_ERROR : 0;
}

void perror(const char *s)
{
	if (s)
	{
		vfsPuts(s, stderr);
		
		vfsPutc(':', stderr);
		vfsPutc(' ', stderr);
	}

	const char *err = __err_str[errno];
	vfsPuts(err, stderr);
	vfsPutc('\n', stderr);
}

int remove(const char *fname)
{
	if (!fname)
	{
		errno = ENOENT;
		return EOF;
	}

	char* path = resolve_path(fname);

	if (vfsRemove(path ? path : fname))
	{
		errno = ENOENT;
		return EOF;
	}

	free(path);

	return 0;
}

int rename(const char *old_filename, const char *new_filename)
{
	if (!old_filename || !new_filename)
	{
		errno = EINVAL;
		return EOF;
	}

	char* path_old = resolve_path(old_filename);
	char* path_new = resolve_path(new_filename);

	int returnCode = vfsRename(path_old ? path_old : old_filename, path_new ? path_new : new_filename);

	free(path_old);
	free(path_new);

	return returnCode;
}

FILE *tmpfile()
{
	static const char *tmpdir = "/tmp/";

	DIR *dir = vfsOpendir("/tmp");
	if (!dir) // Create tmp dir
	{
		if (vfsMkdir("/tmp"))
			return NULL; // Couldn't create tmp dir
	}
	else
		vfsClosedir(dir);

	// Find filename
	char *fname = kmalloc(FILENAME_MAX + 5);
	if (!tmpnam(fname + 5))
	{
		kfree(fname);
		return NULL;
	}

	// Prepend directory
	for (int i = 0; i < 5; i++)
		fname[i] = tmpdir[i];

	FILE *file = vfsOpen(fname, "wb+");

	kfree(fname);

	if (!file)
		errno = ENOSPC;

	return file;
}

char *tmpnam(char *filename)
{
	// If filename is NULL a internal buffer should be used
	static char staticFilename[FILENAME_MAX + 1];
	if (!filename)
		filename = staticFilename;

	DIR *tmpdir = vfsOpendir("/tmp");
	if (!tmpdir) // Create tmp dir
	{
		if (vfsMkdir("/tmp"))
			return NULL; // Couldn't create tmp dir

		tmpdir = vfsOpendir("/tmp");
		if (!tmpdir)
			return NULL;
	}

	dirent *entry;

	// Check all available tmp filenames
	for (size_t i = 0; i < SIZE_MAX; ++i)
	{
		// Construct filename (eg. tmp_000014AF)
		snprintf(filename, FILENAME_MAX, "tmp_%.8zX", i);
		bool found = false;

		// Check if already existent
		while((entry = vfsReaddir(tmpdir)))
		{
			if (strcmp(entry->d_name, filename) == 0)
			{
				found = true;
				break;
			}
		}

		tmpdir->index = 0; // Start from beginning

		if (!found)
			return filename;
	}

	return NULL;
}
