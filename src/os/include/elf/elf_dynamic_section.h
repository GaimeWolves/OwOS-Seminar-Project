#ifndef _ELF_DYNAMIC_H
#define _ELF_DYNAMIC_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define DST_NULL		0
#define DST_NEEDED		1
#define DST_PLTRELSZ	2
#define DST_PLTGOT		3
#define DST_HASH		4
#define DST_STRTAB		5
#define DST_SYMTAB		6
#define DST_RELA		7
#define DST_RELASZ		8
#define DST_RELAENT		9
#define DST_STRSZ		10
#define DST_SYMENT		11
#define DST_INIT		12
#define DST_FINI		13
#define DST_SONAME		14
#define DST_RPATH		15
#define DST_SYMBOLIC	16
#define DST_REL			17
#define DST_RELSZ		18
#define DST_RELENT		19
#define DST_PLTREL		20
#define DST_DEBUG		21
#define DST_TEXTREL		22
#define DST_JMPREL		23

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	uint32_t type;
	union
	{
		uint32_t val;
		uint32_t ptr;
	} value;
	
} __attribute__((packed)) ELF_dynamic_section_entry_t;


//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------


#endif // _ELF_DYNAMIC_H
