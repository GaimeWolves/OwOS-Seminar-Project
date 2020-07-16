#ifndef _STLIB_H
#define _STLIB_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdbool.h>
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

char *uitoa(size_t num, char * buf, size_t base, bool prepend_zeros);
char *itoa(int num, char *buf, size_t base, bool prepend_zeros);
char *ulltoa(unsigned long long num, char *buf, size_t base, bool prepend_zeros);
char *lltoa(long long num, char *buf, size_t base, bool prepend_zeros);

unsigned long long strtoull(const char *str, char **str_end, int base);
unsigned long strtoul(const char *str, char **str_end, int base);
long long strtoll(const char *str, char **str_end, int base);
long strtol(const char *str, char **str_end, int base);

long long atoll(const char *str);
long atol(const char *str);
int atoi(const char *str);

void* malloc(size_t size);
void free(void *ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void *ptr, size_t size);

void srand(unsigned seed);
int rand();

#endif //_STLIB_H
