#ifndef _ELF_HEADER_H
#define _ELF_HEADER_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define H_MAGIC_NUMBER	0x464C457f // 0x7f + 'ELF'

#define HC_NONE			0
#define HC_32			1
#define HC_64			2

#define HD_NONE			0
#define HD_LITTLE_ENDIAN	1
#define HD_BIG_ENDIAN	2

#define HT_NONE			0
#define HT_REL			1
#define HT_EXEC			2
#define HT_DYN			3
#define HT_CORE			4

#define HA_NONE			0
#define HA_M32			1
#define HA_SPARC		2
#define HA_386			3
#define HA_68K			4
#define HA_88K			5
#define HA_860			7
#define HA_MIPS			8

#define HV_NONE			0
#define HV_CURRENT		1


#define PHT_NULL		0
#define PHT_LOAD		1
#define PHT_DYNAMIC		2
#define PHT_INTERP		3
#define PHT_NOTE		4
#define PHT_SHLIB		5
#define PHT_PHDR		6


#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11

#define SHF_WRITE		0x01
#define SHF_ALLOC		0x02
#define SHF_EXEC		0x04

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	uint32_t magic_number;
	uint8_t elf_class;
	uint8_t data;
	uint8_t header_version;
	uint8_t os_abi;
	uint64_t unused;
	uint16_t type;
	uint16_t architecture;
	uint32_t version;
	uint32_t entry_point;
	uint32_t program_header_table_pos;
	uint32_t section_header_table_pos;
	uint32_t flags;
	uint16_t header_size;
	uint16_t program_header_table_size;
	uint16_t program_header_table_entry_count;
	uint16_t section_header_table_size;
	uint16_t section_header_table_entry_count;
	uint16_t section_header_table_name_index;
} __attribute__((packed)) ELF_header_t;

typedef struct
{
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t undefined;
	uint32_t segment_size;
	uint32_t memory_size;
	uint32_t flags;
	uint32_t alignment;
} __attribute__((packed)) ELF_program_header_entry_t;

typedef struct
{
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entry_size;
} __attribute__((packed)) ELF_section_header_entry_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------


#endif // _ELF_HEADER_H
