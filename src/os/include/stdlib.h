#pragma once

#include <stdbool.h>
#include <stddef.h>

char* uitoa(size_t num, char* buf, size_t base, bool prepend_zeros);
char* itoa(int num, char* buf, size_t base, bool prepend_zeros);
void* malloc(size_t size);
void free(void *ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void *ptr, size_t size);
void* reallocarray(void *ptr, size_t nmemb, size_t size);
