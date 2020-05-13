#include <multiboot.h>

#include <stdint.h>
#include <stdnoreturn.h>
#include <hal.h>
#include <hal/keyboard.h>
#include <hal/interrupt.h>
#include <keyboard.h>

void write_string(char colour, const char *string, char* buf)
{
	volatile char *video = buf;
	while(*string != 0)
	{
		*video++ = *string++;
		*video++ = colour;
	}
}

_Noreturn void main(uint32_t magic, multiboot_info_t *boot_info)
{
	initHAL();
	setInterruptFlag();
	char* x = 0xB8000;
	while (1) {
		char bla = 0;
		getc(&bla);
		while(!bla);
		char res[] = {bla, 0};
		write_string(0x0f, res, x);
		x += 2;
	}

	// The kernel should never return
	for (;;);	
}
