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

void* memmove(void* dest, const void* src, size_t n) {
	const char* sp = src;
	char* dp = dest;
	// If src is less than dest start at end
	if (src < dest) {
		for (int i = n-1; i >= 0; i--) {
			dp[i] = sp[i];
		}
	}
	// If src is greater than dest start at beginning
	else if (src > dest) {
		for (size_t i = 0; i < n; i++) {
			dp[i] = sp[i];
		}
	}
	// (else) nothing to do, src and dest are the same memory regions
	return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
	const char* c1 = s1;
	const char* c2 = s2;
	for (size_t i = 0; i < n; i++) {
		if (c1[i] != c2[i])
			return c1[i]-c2[i];
	}
	return 0;
}
