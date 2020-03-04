#pragma once
#include <size_t.h>
size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* d, int c, size_t n);
