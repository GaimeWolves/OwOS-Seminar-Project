#include <stdnoreturn.h>

#include <stdint.h>

void write_string(char colour, const char *string)
{
	volatile char *video = (volatile char*)0xB8000;
	while(*string != 0)
	{
		*video++ = *string++;
		*video++ = colour;
	}
}

_Noreturn void main(uint32_t magic, void* boot_info)
{
	write_string(0x0F, "Something went wrong in compilation, this is just a stub to prevent the bootloader from crashing.");
	for(;;);
}
