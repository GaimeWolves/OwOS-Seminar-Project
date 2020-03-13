#include <hal/cpu.h>

void halt(void)
{
	asm volatile ("hlt"::);
}

char* vendorName(void)
{
	return "FIXME: NOT IMPLEMENTED";
}