#include <stdlib.h>

char* uitoa(size_t num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	size_t i = 0;
	do {
		size_t r = num % base;
		buf[i++] = (r > 9) ? (r-10) + 'A' : r + '0';
		num /= base;
	} while (num > 0 || prepend_zeros && ((base == 16 && i < 8) || (base == 2 && i < 32)));

	size_t end = i-1;
	for (size_t k = 0; k <= end/2; k++) {
		char c = buf[k];
		buf[k] = buf[end-k];
		buf[end-k] = c;
	}

	buf[end+1] = 0;
	return buf;
}

char* itoa(int num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	if (num < 0) {
		*buf = '-';
		num *= -1;
	}

	uitoa(num, buf+1, base, prepend_zeros);
	return buf;
}
