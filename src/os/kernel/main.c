#include <multiboot.h>

#include <hal.h>
#include <hal/interrupt.h>
#include <memory/pmm.h>
#include <keyboard.h>
#include <vfs/vfs.h>

_Noreturn void main(uint32_t magic, multiboot_info_t *boot_info)
{
	initHAL();
	initPMM(boot_info);
	initKeyboard();
	initVFS();
	setInterruptFlag();
	
	// The kernel should never return
	for (;;);
}
