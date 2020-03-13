#include <multiboot.h>

#include <stdint.h>
#include <stdnoreturn.h>

_Noreturn void main(uint32_t magic, multiboot_info_t *boot_info)
{
	// The kernel should never return
	for (;;);	
}
