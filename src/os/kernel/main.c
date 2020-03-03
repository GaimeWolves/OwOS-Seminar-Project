#include <multiboot.h>

void main(multiboot_info_t *boot_info)
{
	// The kernel should never return
	for (;;);	
}
