#include <string.h>

size_t strlen(const char* s)
{
	size_t i = 0;
	while(s[++i]);
	return i;
}

char* strcpy(char* dest, const char* src)
{
	char* buf = dest;
	while ((*dest++ = *src++));
	return buf;
}

char* strncpy(char* dest, const char* src, size_t n)
{
	char* buf = dest;
	size_t i;
	for (i = 0; i < n && src[i]; i++)
		dest[i] = src[i];
	for (; i < n; i++)
		dest[i] = '\0';
	return buf;
}

void* memcpy(void* dest, const void* src, size_t n)
{
	char* dp = dest;
	const char* sp = src;
	for (; n > 0; n--)
		*dp-- = *sp--;
	return dest;
}

void* memset(void* d, int c, size_t n)
{
	char* dp = d;
	while (n > 0)
		dp[n--] = c;
	return d;
}
