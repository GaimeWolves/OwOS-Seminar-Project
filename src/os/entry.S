#define ASM_FILE 1
#include <multiboot.h>

#define MULTIBOOT_HEADER_FLAGS	(MULTIBOOT_HEADER_PAGE_ALIGN | MULTIBOOT_HEADER_MEMORY | MULTIBOOT_HEADER_AOUT_KLUDGE | MULTIBOOT_HEADER_VIDEO_MODE)
#define STACK_SIZE 0x4000

.section .multiboot
.global entry
entry:
	jmp multiboot_entry

.align 4  // Align on 32 bits boundary
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS

	// Checksum ( checksum + magic + flags = 0 )
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	
	// Address settings
	.long multiboot_header  // Header address
	.long entry             // Loading address
	.long _edata            // Data end address
	.long _end              // OS end address
	.long multiboot_entry   // Entry address

	// Video mode settings
	.long 1   // Text mode
	.long 80  // Columns
	.long 25  // Rows
	.long 0   // Bit depth (0 for text mode)

.section .text
multiboot_entry:
	// Set stack
	movl $_estack, %esp
	movl %esp, %ebp

	// Reset EFLAGS
	pushl $0
	popf

	push %ebx
	push %eax

	call main

	cli
	hlt
