#include <multiboot.h>

#include <hal.h>
#include <hal/interrupt.h>
#include <memory/pmm.h>
#include <keyboard.h>

_Noreturn void main(uint32_t magic, multiboot_info_t *boot_info)
{
	initHAL();
	initPMM(boot_info);
	initKeyboard();
	setInterruptFlag();
	
	// The kernel should never return
	for (;;);
}
