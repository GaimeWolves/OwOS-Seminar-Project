#pragma once
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

//GNU Assembly doesn't like typedefs
#ifndef ASM_FILE
#include <stdint.h>
#endif

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

// Signatures
#define MULTIBOOT_HEADER_MAGIC     0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// Flags defined by OS
#define MULTIBOOT_HEADER_PAGE_ALIGN  0x00000001 // Align boot modules on page boundary
#define MULTIBOOT_HEADER_MEMORY      0x00000002 // Memory information required
#define MULTIBOOT_HEADER_VIDEO_MODE  0x00000004 // Video information required
#define MULTIBOOT_HEADER_AOUT_KLUDGE 0x00010000 // Address fields set in header

// Flags defined by Bootloader
#define MULTIBOOT_INFO_MEMORY           0x0000001 // Memory count available
#define MULTIBOOT_INFO_BOOTDEVICE       0x0000002 // Boot device set
#define MULTIBOOT_INFO_CMDLINE          0x0000004 // Command line defined
#define MULTIBOOT_INFO_MODULES          0x0000008 // Modules loaded
#define MULTIBOOT_INFO_AOUT_SYMS        0x0000010 // Symbol table available
#define MULTIBOOT_INFO_ELF_SHEADER      0x0000020 // ELF section header available
#define MULTIBOOT_INFO_MEM_MAP          0x0000040 // Full memory map available
#define MULTIBOOT_INFO_DRIVE_INFO       0x0000080 // Drive info available
#define MULTIBOOT_INFO_CONFIG_TABLE     0x0000100 // Config table available
#define MULTIBOOT_INFO_BOOTLOADER_NAME  0x0000200 // Bootloader name set
#define MULTIBOOT_INFO_APM_TABLE        0x0000400 // APM table available
#define MULTIBOOT_INFO_VBE_INFO         0x0000800 // Video information available
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO 0x0001000

// Memory map region types
#define MMAP_AVAILABLE 1
#define MMAP_RESERVED  2
#define MMAP_ACPI      3
#define MMAP_NVS       4
#define MMAP_BADRAM    5

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//GNU Assembly doesn't has no typedefs
#ifndef ASM_FILE

typedef struct
{
	uint32_t tabsize;
	uint32_t strsize;
	uint32_t addr;
	uint32_t reserved;
} __attribute__((packed)) multiboot_aout_syms_t;

// Represents the multiboot info struct
typedef struct
{
	uint32_t flags;
	uint32_t memory_lo;
	uint32_t memory_hi;
	uint32_t boot_device;
	uint32_t cmd_ln;
	uint32_t mods_cnt;
	uint32_t mods_addr;
	multiboot_aout_syms_t syms;
	uint32_t mmap_len;
	uint32_t mmap_addr;
	uint32_t drives_len;
	uint32_t drives_addr;
	uint32_t conf_table;
	uint32_t boot_name;
	uint32_t apm_table;
	uint32_t vbe_ctrl_info;
	uint32_t vbe_mode_info;
	uint16_t vbe_io_seg;
	uint32_t vbe_io_off;
	uint16_t vbe_io_len;
} __attribute__((packed)) multiboot_info_t;

// Represents an entry in the memory map
typedef struct
{
	uint32_t size;
	uint64_t addr;
	uint64_t len;
	uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

#endif //ASM_FILE
