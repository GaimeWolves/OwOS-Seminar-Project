#pragma once
#include <stdbool.h>
#include <size_t.h>
char* uitoa(size_t num, char* buf, size_t base, bool prepend_zeros);
char* itoa(int num, char* buf, size_t base, bool prepend_zeros);
