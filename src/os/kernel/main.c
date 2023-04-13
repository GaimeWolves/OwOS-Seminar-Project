#include <multiboot.h>

#include <hal.h>
#include <hal/interrupt.h>
#include <memory/pmm.h>
#include <keyboard.h>
#include <vfs/vfs.h>
#include <shell/shell.h>
#include <debug.h>

_Noreturn void main(uint32_t magic, multiboot_info_t *boot_info)
{
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		//The bootloader is not multiboot compliant
		//We can't work with this
		for(;;);
	}

	initHAL();
	initPMM(boot_info);
	initKeyboard();
	initVFS();
	setInterruptFlag();

	//Initialize shell
	shell_init();
	//Start shell loop
	//We will not return
	shell_start();
	
	// The kernel should never return
	for (;;);
}
