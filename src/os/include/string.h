#ifndef _STRING_H_
#define _STRING_H_

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stddef.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t count);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, size_t count);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t count);
char *strstr(const char *cs, const char *ct);

int memcmp(const void *s1, const void *s2, size_t len);
void* memcpy(void *to, const void *from, size_t n);
void* memset(void *s, char c, size_t count);
void *memmove(void *dest, const void *src, size_t n);
void *memchr(const void *cs, int c, size_t count);
void *memscan(void *addr, int c, size_t size);

#endif