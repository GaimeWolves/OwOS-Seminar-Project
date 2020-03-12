#include <stdnoreturn.h>

void write_string(char colour, const char *string)
{
	volatile char *video = (volatile char*)0xB8000;
	while(*string != 0)
	{
		*video++ = *string++;
		*video++ = colour;
	}
}

noreturn void entry()
{
	write_string(0x0F, "Something went wrong in compilation, this is just a stub to prevent the bootloader from crashing.");
	for(;;);
}
