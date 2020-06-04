#include "relocation.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <ld-owos/ld-owos.h>
#include <elf/elf_relocation.h>
#include <elf/elf_symbol_table.h>
#include "dynamic-linker.h"

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef union relocation_entry
{
	ELF_Rel rel;
	ELF_Rela rela;
} relocation_entry_t;

//------------------------------------------------------------------------------------------
//				Macro
//------------------------------------------------------------------------------------------

//Gets the addend of the relocation entry
//type == RELOC_REL  = *address
//type == RELOC_RELA = addend specified in ELF_Rela struct
#define RELOCATION_ADDEND(type, relocation_entry, address) (type == RELOC_REL ? *address : relocation_entry->rela.addend)
//Increses the relocation entry to the next one in memory
//Used for better code appearance because of the ugly casts
#define INCREASE_RELOCATION_ENTRY(relocation_entry, entry_size) relocation_entry = (relocation_entry_t*)((size_t)relocation_entry + entry_size)

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------

//Process the relocation of a given dynamic_linker_reloc_info_t struct
//EXCEPTIONS:
//	-1: Unknown/Bad reloc type
//	-2: Bad relocation entry size
//	-3: Unable to handle entry
int process_relocation(libinfo_t* libinfo, dynamic_linker_reloc_info_t info)
{
	//If type if not REL or RELA report error
	if(info.type != RELOC_REL && info.type != RELOC_RELA)
		return -1;

	//If the sizes don't match then something is wrong
	if(	info.entry_size != sizeof(ELF_Rela)
		&& info.entry_size != sizeof(ELF_Rel))
		return -2;

	//Loop through the table
	relocation_entry_t* entry = (relocation_entry_t*)info.address;
	while((size_t)entry - (size_t)info.address < info.size)
	{
		//Get vars out of the entry
		uint32_t* address = RESOLVE_MEM_ADDRESS(libinfo, entry->rel.offset);
		uint8_t type = ELF32_R_TYPE(entry->rel.info);
		size_t index = ELF32_R_SYM(entry->rel.info);

		//get compile time address out of according symbol table entry
		uint32_t compile_time_address;
		uint32_t load_time_address;
		if(index)
		{
			compile_time_address = libinfo->symbol_table_base[index].value;
			
			//Test if this entry needs another lib
			if (compile_time_address == 0)
			{
				dynamic_linker_add_dynamic_linking(libinfo, address, libinfo->symbol_table_base[index].name, type, RELOCATION_ADDEND(info.type, entry, address));
				INCREASE_RELOCATION_ENTRY(entry, info.entry_size);
				continue;
			}

			load_time_address = RESOLVE_MEM_ADDRESS(libinfo, compile_time_address);
		}
	
		//Handling depends on the type
		switch (type)
		{
			case R_386_32:
				*address = load_time_address + RELOCATION_ADDEND(info.type, entry, address);
				break;
			case R_386_PC32:
				*address = load_time_address + RELOCATION_ADDEND(info.type, entry, address) - (uint32_t)(address);
				break;
			case R_386_JMP_SLOT:
			case R_386_GLOB_DAT:
				*address = load_time_address;
				break;
			case R_386_RELATIVE:
				*address = libinfo->base_address + RELOCATION_ADDEND(info.type, entry, address);
				break;
			case R_386_GOTOFF:
				*address = load_time_address + RELOCATION_ADDEND(info.type, entry, address) - (size_t)libinfo->plt_got_address;
				break;
			case R_386_GOTPC:
				*address = (size_t)libinfo->plt_got_address + RELOCATION_ADDEND(info.type, entry, address) - (uint32_t)(address);
				break;
			case R_386_GOT32:
			case R_386_PLT32:
			case R_386_COPY:
			default:
				return -3;

			case R_386_NONE:
				break;
		}

		INCREASE_RELOCATION_ENTRY(entry, info.entry_size);
	}

	return 0;
}