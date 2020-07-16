#ifndef _LD_OWOS_DYNMAIC_H
#define _LD_OWOS_DYNMAIC_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <ld-owos/ld-owos.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

//Process the dynamic section of a library
//EXCEPTIONS:
//	- 1: Could not handle needed dynamic entry type
//	- 2: Could not get needed dynamic section entries
//	-10: process_relocation 
//	-20: resolve_symbols
int process_dynamic_section(libinfo_t* libinfo, ELF_program_header_entry_t* entry);
//Add an entry to the needed dynamic linked symbol linklist
void dynamic_linker_add_dynamic_linking(libinfo_t* libinfo, uint32_t* address, size_t name_index, uint8_t type, uint32_t addend);

#endif