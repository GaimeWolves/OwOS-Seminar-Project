#include <multiboot.h>

extern void main(multiboot_info_t *boot_info);

void entry(multiboot_info_t *boot_info)
{
	main(boot_info);
}
