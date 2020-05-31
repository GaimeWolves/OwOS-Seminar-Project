#include <stdio.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define CONVERSION_FLAG_LADJUST   0x01  // Left adjust
#define CONVERSION_FLAG_SIGN      0x02  // Always print sign
#define CONVERSION_FLAG_SPACE     0x04  // Print space on positive value
#define CONVERSION_FLAG_ALT       0x08  // Alternative representation
#define CONVERSION_FLAG_ZERO      0x10  // Adjust with zeroes
#define CONVERSION_FLAG_PRECISION 0x40  // Precision explicitly specified
#define CONVERSION_FLAG_PARSING   0x80  // Used in conversion

// Default conversion length: int
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

// Holds the state of the conversion for use in helper functions
typedef struct
{
	// State of current conversion
	uint8_t flags;
	int minimal_width;
	int precision;
	uint8_t length;
	char specifier;

	// State of current printf call
	const char **format;
	char **buffer;
	size_t *bufsz;
	int *written;
	va_list *ap;
} conversion_t;

//------------------------------------------------------------------------------------------
//				Private function declarations
//------------------------------------------------------------------------------------------

static void write_padding(conversion_t *conversion, bool ladjust, char fill);
static void write_string(conversion_t *conversion, const char *s, size_t size);
static void write_char(conversion_t *conversion, const char c);
static void write_char_seq(conversion_t *conversion, const char c, size_t count);

static void parse_number(conversion_t *conversion, char *buffer, void* var);
static void parse_varg(conversion_t *conversion, char *buffer);
static void parse_flags(conversion_t *conversion);
static void parse_width(conversion_t *conversion);
static char parse_precision(conversion_t *conversion);
static void parse_length(conversion_t *conversion);

//------------------------------------------------------------------------------------------
//				Private function implementations
//------------------------------------------------------------------------------------------

// Helper function printing the padding for the conversion
static void write_padding(conversion_t *conversion, bool ladjust, char fill)
{
	if (conversion->minimal_width <= 0)
		return;

	if (ladjust != (conversion->flags & CONVERSION_FLAG_LADJUST))
		return;

	write_char_seq(conversion, fill, conversion->minimal_width);
}

// Helper function to write a string to the buffer
// updating the bufsz and written variables
static void write_string(conversion_t *conversion, const char *s, size_t size)
{
	*conversion->written += size;
	size = size > *conversion->bufsz - 1 ? *conversion->bufsz - 1 : size;

	strncpy(*conversion->buffer, s, size);
	*conversion->buffer += size;
	*conversion->bufsz -= size;
}

// Helper function to write a char to the buffer
// updating the bufsz and written variables
static void write_char(conversion_t *conversion, const char c)
{
	(*conversion->written)++;

	if (*conversion->bufsz > 1)
	{
		*(*conversion->buffer)++ = c;
		(*conversion->bufsz)--;
	}
}

static void write_char_seq(conversion_t *conversion, const char c, size_t count)
{
	*conversion->written += count;

	count = count > *conversion->bufsz - 1 ? *conversion->bufsz - 1 : count;

	while (count--)
	{
		*(*conversion->buffer)++ = c;
		(*conversion->bufsz)--;
	}
}

// Helper function to parse value by conversion specifier
// Pointer gets cast to signed or unsigned type accordingly
static void parse_number(conversion_t *conversion, char* buffer, void* var)
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
static void parse_varg(conversion_t *conversion, char* buffer)
{
	// Max value gets cast to needed value
	uint64_t var;

	switch(conversion->length)
	{
		case CONVERSION_LENGTH_HH:
		{
			// First convert to int because of integer promotion in var args
			var = (char)va_arg(*conversion->ap, int);
			break;
		}
		case CONVERSION_LENGTH_H:
		{
			// First convert to int because of integer promotion in var args
			var = (short)va_arg(*conversion->ap, int);
			break;
		}
		case CONVERSION_LENGTH_NONE:
		{
			var = va_arg(*conversion->ap, int);
			break;
		}
		case CONVERSION_LENGTH_L:
		{
			var = va_arg(*conversion->ap, long);
			break;
		}
		case CONVERSION_LENGTH_LL:
		{
			var = va_arg(*conversion->ap, unsigned long long);
			break;
		}
		case CONVERSION_LENGTH_J:
		{
			var = va_arg(*conversion->ap, uint64_t);
			break;
		}
		case CONVERSION_LENGTH_Z:
		{
			var = va_arg(*conversion->ap, size_t);
			break;
		}
		case CONVERSION_LENGTH_T:
		{
			var = va_arg(*conversion->ap, ptrdiff_t);
			break;
		}
		default:
			break;
	}

	parse_number(conversion, buffer, &var);
}

// Helper function to parse the flags for the conversion
static void parse_flags(conversion_t *conversion)
{
	while((*conversion->format)[0] && conversion->flags & CONVERSION_FLAG_PARSING)
	{
		switch(*(conversion->format)[0])
		{
			case '-':
			{
				conversion->flags |= CONVERSION_FLAG_LADJUST;
				break;
			}
			case '+':
			{
				conversion->flags |= CONVERSION_FLAG_SIGN;
				break;
			}
			case ' ':
			{
				conversion->flags |= CONVERSION_FLAG_SPACE;
				break;
			}
			case '#':
			{
				conversion->flags |= CONVERSION_FLAG_ALT;
				break;
			}
			case '0':
			{
				conversion->flags |= CONVERSION_FLAG_ZERO;
				break;
			}
			default:
			{
				// Clear parsing flag
				conversion->flags ^= CONVERSION_FLAG_PARSING;
				(*conversion->format)--; // So we don't skip anything
				break;
			}
		}
		(*conversion->format)++;
	}
}

// Helper function to parse the specified minimal width
static void parse_width(conversion_t *conversion)
{
	if ((*conversion->format)[0] == '*')
	{
		conversion->minimal_width = va_arg(*conversion->ap, int);
		(*conversion->format)++;
	}
	else if (isdigit(*conversion->format[0]))
	{
		char* str_end;
		conversion->minimal_width = (int)strtol(*conversion->format, &str_end, 10);
		*conversion->format = str_end;
	}

	if (conversion->minimal_width < 0)
	{
		conversion->flags |= CONVERSION_FLAG_LADJUST;
		conversion->minimal_width *= -1;
	}
}

// Helper function to parse the specified precision
// Returns the character used for padding a converted integer
static char parse_precision(conversion_t *conversion)
{
	char fill = ' ';

	if ((*conversion->format)[0] == '.')
	{
		(*conversion->format)++;
		conversion->flags |= CONVERSION_FLAG_PRECISION;
		
		if ((*conversion->format)[0] == '*')
		{
			conversion->precision = va_arg(*conversion->ap, int);
			(*conversion->format)++;
		}
		else if (isdigit(*conversion->format[0]))
		{
			char* str_end;
			conversion->precision = (int)strtol(*conversion->format, &str_end, 10);
			*conversion->format = str_end;
		}

		if (conversion->precision < 0)
			conversion->precision = 0;
	}
	else if (conversion->flags & CONVERSION_FLAG_ZERO)
		fill = '0'; // Use '0' as padding only if precision not specified

	return fill;
}

// Helper function to parse the length modifier
static void parse_length(conversion_t *conversion)
{
	switch((*conversion->format)[0])
	{
		case 'h':
		{
			if ((*conversion->format)[1] == 'h')
			{
				conversion->length = CONVERSION_LENGTH_HH;
				(*conversion->format)++;
			}
			else
				conversion->length = CONVERSION_LENGTH_H;
			break;
		}
		case 'l':
		{
			if ((*conversion->format)[1] == 'l')
			{
				conversion->length = CONVERSION_LENGTH_LL;
				(*conversion->format)++;
			}
			else
				conversion->length = CONVERSION_LENGTH_L;
			break;
		}
		case 'j':
		{
			conversion->length = CONVERSION_LENGTH_J;
			break;
		}
		case 'z':
		{
			conversion->length = CONVERSION_LENGTH_Z;
			break;
		}
		case 't':
		{
			conversion->length = CONVERSION_LENGTH_T;
			break;
		}
		default:
		{
			(*conversion->format)--; // So we don't skip anything
			break;
		}
	}
	(*conversion->format)++;
}

//------------------------------------------------------------------------------------------
//				Public function implementations
//------------------------------------------------------------------------------------------

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

int vsprintf(char *buffer, const char *format, va_list ap)
{
	// To prevent code duplication the maximum return value
	// for the architecture is used	
	return vsnprintf(buffer, INT_MAX, format, ap);
}

int vsnprintf(char *buffer, size_t bufsz, const char *format, va_list ap)
{
	if (!buffer || !format)
		return -1;

	int written = 0;

	while(format[0] && bufsz > 1)
	{
		if (format[0] == '%')
		{
			conversion_t conversion = { CONVERSION_FLAG_PARSING, 0, 1, 0, 0, &format, &buffer, &bufsz, &written, &ap };
			format++;

			// Parse optional flags
			parse_flags(&conversion);

			// Parse minimal width modifier
			parse_width(&conversion);

			// Parse precision
			// Character used for padding (only applies to integer conversion)
			char fill = parse_precision(&conversion);

			// Parse length modifier
			parse_length(&conversion);

			// Parse format specifier
			conversion.specifier = format[0];
			switch (conversion.specifier)
			{
				case '%': // Print % symbol
				{
					// Full specifier must be %%
					if (conversion.flags | conversion.length | conversion.minimal_width | conversion.precision)
						break;

					write_char(&conversion, '%');

					break;
				}
				case 'c': // Print char
				{
					written++;

					// Specification says to first convert to unsigned char
					unsigned char c = (unsigned char)va_arg(ap, int);
					
					conversion.minimal_width--;
					write_padding(&conversion, false, ' ');
					
					write_char(&conversion, c);

					write_padding(&conversion, true, ' ');

					break;
				}
				case 's': // Print string
				{
					char* s = va_arg(ap, char*);
					size_t size = strlen(s);
					
					// Precision declares maximum characters to be printed
					if (conversion.flags & CONVERSION_FLAG_PRECISION)
						size = size > (size_t)conversion.precision ? (size_t)conversion.precision : size;

					conversion.minimal_width -= size;
					write_padding(&conversion, false, ' ');

					write_string(&conversion, s, size);
					
					write_padding(&conversion, true, ' ');

					break;
				}
				case 'd': // Print signed decimal
				case 'i':
				case 'u': // Print unsigned decimal
				{
					// Decimal representation takes up at most 21 characters
					char repr[21] = { 0 };
					parse_varg(&conversion, repr);
					
					size_t size = strlen(repr);

					// Sign char calculated based on sign flag and sign of converted number
					char sign = 0;
					if (conversion.specifier != 'u')
					{
						if (repr[0] == '-')
						{
							memmove((void*)repr, (void*)(repr + 1), size);
							sign = '-';
							size--;
						}
						else if (conversion.flags & CONVERSION_FLAG_SIGN)
							sign = '+';
						else if (conversion.flags & CONVERSION_FLAG_SPACE)
							sign = ' ';
					}
				
					// Padding calculated depending on precision and zero flag
					size_t padding = 0;
					if (~conversion.flags & CONVERSION_FLAG_PRECISION && conversion.flags & CONVERSION_FLAG_ZERO)
						padding = size > (size_t)conversion.minimal_width ? 0 : conversion.minimal_width - size;
					else
						padding = size > (size_t)conversion.precision ? 0 : conversion.precision - size;

					// If precision and value both equal 0 nothing is printed
					if (conversion.precision > 0 || repr[0] != '0')
						conversion.minimal_width -= size + padding;

					write_padding(&conversion, false, fill);

					// Only print if precision and representation not zero
					if (conversion.precision > 0 || repr[0] != '0')
					{
						if (sign)
							write_char(&conversion, sign);

						write_char_seq(&conversion, '0', padding);
						write_string(&conversion, repr, size);
					}

					write_padding(&conversion, true, fill);

					break;
				}
				case 'o': // Print octal number
				{
					// Octal representation takes up at most 22 characters
					char repr[22] = { 0 };
					parse_varg(&conversion, repr);
					
					size_t size = strlen(repr);
					
					// Padding calculated depending on precision and zero flag
					size_t padding = 0;
					if (~conversion.flags & CONVERSION_FLAG_PRECISION && conversion.flags & CONVERSION_FLAG_ZERO)
						padding = size > (size_t)conversion.minimal_width ? 0 : conversion.minimal_width - size;
					else
						padding = size > (size_t)conversion.precision ? 0 : conversion.precision - size;

					// If precision and value both equal 0 nothing is printed
					if (conversion.precision > 0 || repr[0] != '0')
						conversion.minimal_width -= size + padding;

					// Alternative representation reqires at least one leading zero
					if (conversion.flags & CONVERSION_FLAG_ALT)
						conversion.minimal_width--;

					write_padding(&conversion, false, fill);	

					// Alternative representation prints a preceeding zero
					if (conversion.flags & CONVERSION_FLAG_ALT)
						write_char(&conversion, '0');
					
					// Only print if precision and representation not zero
					if (conversion.precision > 0 || repr[0] != '0')
					{
						write_char_seq(&conversion, '0', padding);
						write_string(&conversion, repr, size);
					}

					write_padding(&conversion, true, fill);

					break;
				}
				case 'x': // Print hexadecimal with lowercase letters
				case 'X': // with uppercase letters
				{
					// Hexadecimal representation takes up at most 17 characters
					char repr[17] = { 0 };
					parse_varg(&conversion, repr);
					
					size_t size = strlen(repr);

					// Upper or lower case depending in specifier;
					for (size_t i = 0; i < size; i++)
						repr[i] = islower(conversion.specifier) ? tolower(repr[i]) : repr[i];

					// Padding calculated depending on precision and zero flag
					size_t padding = 0;
					if (~conversion.flags & CONVERSION_FLAG_PRECISION && conversion.flags & CONVERSION_FLAG_ZERO)
						padding = size > (size_t)conversion.minimal_width ? 0 : conversion.minimal_width - size;
					else
						padding = size > (size_t)conversion.precision ? 0 : conversion.precision - size;

					if (conversion.precision > 0 || repr[0] != '0')
						conversion.minimal_width -= size + padding;

					if (conversion.flags & CONVERSION_FLAG_ALT && repr[0] != '0')
						conversion.minimal_width -= 2;

					write_padding(&conversion, false, fill);	

					// Alternative representation prints 0x before the value
					if (conversion.flags & CONVERSION_FLAG_ALT && repr[0] != '0')
					{
						write_char(&conversion, '0');
						write_char(&conversion, conversion.specifier); // x or X depending on specifier
					}

					// If precision and value both equal 0 nothing is printed
					if (conversion.precision > 0 || repr[0] != '0')
					{
						write_char_seq(&conversion, '0', padding);
						write_string(&conversion, repr, size);
					}

					write_padding(&conversion, true, fill);

					break;
				}
				case 'n': // Store characters written
				{
					void* var = va_arg(ap, void*);

					switch(conversion.length)
					{
						case CONVERSION_LENGTH_HH:
						{	
							*(char*)var = (char)written;
							break;
						}
						case CONVERSION_LENGTH_H:
						{
							*(short*)var = (short)written;
							break;
						}
						case CONVERSION_LENGTH_NONE:
						{
							*(int*)var = (int)written;
							break;
						}
						case CONVERSION_LENGTH_L:
						{
							*(long*)var = (long)written;
							break;
						}
						case CONVERSION_LENGTH_LL:
						{
							*(long long*)var = (long long)written;
							break;
						}
						case CONVERSION_LENGTH_J:
						{
							*(int64_t*)var = (int64_t)written;
							break;
						}
						case CONVERSION_LENGTH_Z:
						{
							*(size_t*)var = (size_t)written;
							break;
						}
						case CONVERSION_LENGTH_T:
						{
							*(ptrdiff_t*)var = (ptrdiff_t)written;
							break;
						}
						default:
							break;
					}

					break;
				}
				case 'p': // Print pointer
				{
					uint32_t ptr = va_arg(ap, uint32_t);
					char repr[11] = { '0', 'x', 0 };
					uitoa((size_t)ptr, repr + 2, 16, true);
					
					conversion.minimal_width -= 10;
					write_padding(&conversion, false, ' ');	

					write_string(&conversion, repr, 10);

					write_padding(&conversion, true, ' ');

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
			written++;
			if (bufsz > 1)
			{
				*buffer++ = format[0];
				bufsz--;
			}
		}

		format++;
	}

	// Terminal with null byte
	if (bufsz)
		*buffer++ = 0;

	// Return number of characters that would've been written without bufsz limit
	return written;
}
