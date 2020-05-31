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
	asm volatile("hlt"::);
}

char* vendorName(void)
{
	return "FIXME: NOT IMPLEMENTED";
}

void outb(uint16_t port, uint8_t data)
{
	asm volatile("outb %0, %1"::"a"(data),"dN"(port));
}
void outw(uint16_t port, uint16_t data)
{
	asm volatile("outw %0, %1"::"a"(data),"dN"(port));
}
void outd(uint16_t port, uint32_t data)
{
	asm volatile("outl %0, %1"::"a"(data),"dN"(port));
}

uint8_t inb(uint16_t port)
{
	uint8_t data = 0;
	asm volatile("inb %1, %0":"=a"(data):"dN"(port));
	return data;
}
uint16_t inw(uint16_t port)
{
	uint16_t data = 0;
	__asm__("inw %1, %0":"=a"(data):"dN"(port));
	return data;
}

uint32_t ind(uint16_t port)
{
	uint32_t data = 0;
	__asm__("inl %1, %0":"=a"(data):"dN"(port));
	return data;
}