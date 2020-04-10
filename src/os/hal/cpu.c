#include <hal/cpu.h>
#include <hal/gdt.h>
#include <string.h>

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Local Vars
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

extern inline void outb(uint16_t port, uint8_t data)
{
	__asm__("out dx, al"::"a"(data),"d"(port));
}
extern inline void outw(uint16_t port, uint16_t data)
{
	__asm__("out dx, ax"::"a"(data),"d"(port));
}
extern inline void outd(uint16_t port, uint32_t data)
{
	__asm__("out dx, eax"::"a"(data),"d"(port));
}