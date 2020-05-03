#include <hal/cpu.h>
#include <hal/gdt.h>
#include <string.h>
//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
void halt(void)
{
	asm volatile ("hlt"::);
}

char* vendorName(void)
{
	return "FIXME: NOT IMPLEMENTED";
}

void outb(uint16_t port, uint8_t data)
{
	__asm__("out dx, al"::"a"(data),"d"(port));
}
void outw(uint16_t port, uint16_t data)
{
	__asm__("out dx, ax"::"a"(data),"d"(port));
}
void outd(uint16_t port, uint32_t data)
{
	__asm__("out dx, eax"::"a"(data),"d"(port));
}

uint8_t inb(uint16_t port)
{
	uint8_t data = 0;
	__asm__("in al, dx":"=a"(data):"d"(port));
	return data;
}
uint16_t inw(uint16_t port)
{
	uint16_t data = 0;
	__asm__("in ax, dx":"=a"(data):"d"(port));
	return data;
}

uint32_t ind(uint16_t port)
{
	uint32_t data = 0;
	__asm__("in eax, dx":"=a"(data):"d"(port));
	return data;
}