#ifndef _LD_OWOS_RELOC_H
#define _LD_OWOS_RELOC_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <ld-owos/ld-owos.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

#define RELOC_NULL	0
#define RELOC_REL	1
#define RELOC_RELA	2

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

//Process the relocation of a given dynamic_linker_reloc_info_t struct
//EXCEPTIONS:
//	-1: Unknown/Bad reloc type
//	-2: Bad relocation entry size
//	-3: Unable to handle entry
int process_relocation(libinfo_t* libinfo, dynamic_linker_reloc_info_t info);

#endif