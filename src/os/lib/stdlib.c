#include <stdlib.h>

#include <ctype.h>

char* ulltoa(unsigned long long num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	size_t i = 0;
	do {
		size_t r = num % base;
		buf[i++] = (r > 9) ? (r - 10) + 'A' : r + '0';
		num /= base;
	} while (num > 0 || (prepend_zeros && ((base == 16 && i < 16) || (base == 2 && i < 64))));

	size_t end = i - 1;
	for (size_t k = 0; k <= end / 2; k++) {
		char c = buf[k];
		buf[k] = buf[end - k];
		buf[end - k] = c;
	}

	buf[end + 1] = 0;
	return buf;
}

char* lltoa(long long num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	if (num < 0) {
		*buf = '-';
		num *= -1;
		ulltoa(num, buf + 1, base, prepend_zeros);
	}
	else
		ulltoa(num, buf, base, prepend_zeros);

	return buf;
}

char* uitoa(size_t num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	size_t i = 0;
	do {
		size_t r = num % base;
		buf[i++] = (r > 9) ? (r - 10) + 'A' : r + '0';
		num /= base;
	} while (num > 0 || (prepend_zeros && ((base == 16 && i < 8) || (base == 2 && i < 32))));

	size_t end = i - 1;
	for (size_t k = 0; k <= end / 2; k++) {
		char c = buf[k];
		buf[k] = buf[end - k];
		buf[end - k] = c;
	}

	buf[end + 1] = 0;
	return buf;
}

char* itoa(int num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	if (num < 0) {
		*buf = '-';
		num *= -1;
		uitoa(num, buf + 1, base, prepend_zeros);
	}
	else
		uitoa(num, buf, base, prepend_zeros);

	return buf;
}

unsigned long long strtoull(const char *str, char **str_end, int base)
{
	// Base should be 0 or between 2 and 36
	if (base < 0 || base == 1 || base > 36)
		return 0;

	if (!str)
		return 0;

	unsigned long long num = 0;

	// Remove preceeding whitespace
	while(isspace(str[0]))
		str++;

	// Recognize base automatically if set to 0
	if (base == 0)
	{
		// If the number starts with zero
		if (str[0] == '0')
		{
			str++;

			// Is it 0x or 0X? -> Hex number
			if (tolower(str[0]) == 'x')
			{
				str++;
				base = 16;
			}
			else // Otherwise octal number
				base = 8;
		}
		else // Otherwise decimal number
			base = 10;
	}

	// Parse the remaining string
	while(str)
	{
		// Break on invalid character
		if (!isalnum(str[0]))
			break;

		int value;

		// Calculate the digit
		if (isdigit(str[0]))
			value = (int)(str[0] - '0');
		else
			value = (int)(toupper(str[0]) - 'A') + 10;

		// Break if digit not in range of base
		if (value >= base)
			break;

		// Shift number one digit to the left and add new digit
		num *= (unsigned long long)base;
		num += (unsigned long long)value;

		str++;
	}

	// Store end of the number in the string
	if (str_end)
		*str_end = (char*)str;

	return num;
}

unsigned long strtoul(const char *str, char **str_end, int base)
{
	return (unsigned long)strtoull(str, str_end, base);
}

long long strtoll(const char *str, char **str_end, int base)
{
	// Remove preceeding whitespace
	while(isspace(str[0]))
		str++;

	// Remove minus
	long long sign = 1;
	if (str[0] == '-')
	{
		str++;
		sign = -1;
	}

	// Multiply parsed number by sign
	return sign * (long long)strtoull(str, str_end, base);
}

long strtol(const char *str, char **str_end, int base)
{
	return (long)strtoll(str, str_end, base);
}

long long atoll(const char *str)
{
	return strtoll(str, NULL, 10);
}

long atol(const char *str)
{
	return strtol(str, NULL, 10);
}

int atoi(const char *str)
{
	return (int)strtol(str, NULL, 10);
}
