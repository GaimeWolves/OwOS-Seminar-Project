#include "relocation.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <ld-owos/ld-owos.h>
#include <elf/elf_relocation.h>
#include <elf/elf_symbol_table.h>
#include "dynamic-linker.h"

//------------------------------------------------------------------------------------------
//				Macro
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function
//------------------------------------------------------------------------------------------

int process_relocation_rel(libinfo_t* libinfo, dynamic_linker_reloc_info_t info, size_t got_address)
{
	//If the sizes don't match then something is wrong
	if(info.entry_size != sizeof(ELF_Rel))
		return -10;

	//Loop through the table
	ELF_Rel* entry = info.address;
	while((size_t)entry - (size_t)info.address < info.size)
	{
		//Get vars out of the entry
		uint32_t* address = RESOLVE_MEM_ADDRESS(libinfo, entry->offset);
		uint8_t type = ELF32_R_TYPE(entry->info);
		size_t index = ELF32_R_SYM(entry->info);

		//get compile time address out of according symbol table entry
		uint32_t compile_time_address;
		uint32_t load_time_address;
		if(index)
		{
			compile_time_address = libinfo->symbol_table_base[index].value;
			
			//Test if this entry needs another lib
			if (compile_time_address == 0)
			{
				dynamic_linker_add_dynamic_linking(address, libinfo->symbol_table_base[index].name, type, libinfo);
				entry++;
				continue;
			}

			load_time_address = RESOLVE_MEM_ADDRESS(libinfo, compile_time_address);
		}
	
		//Handling depends on the type
		switch (type)
		{
			case R_386_32:
				*address = load_time_address + *address;
				break;
			case R_386_PC32:
				*address = load_time_address + *address - (uint32_t)(address);
				break;
			case R_386_JMP_SLOT:
			case R_386_GLOB_DAT:
				*address = load_time_address;
				break;
			case R_386_RELATIVE:
				*address = libinfo->base_address  + *address;
				break;
			case R_386_GOTOFF:
				*address = load_time_address + *address - got_address;
				break;
			case R_386_GOTPC:
				*address = got_address + *address - (uint32_t)(address);
				break;
			case R_386_GOT32:
			case R_386_PLT32:
			case R_386_COPY:
			default:
				return -20;

			case R_386_NONE:
				break;
		}

		entry++;
	}
}
int process_relocation_rela(libinfo_t* libinfo, dynamic_linker_reloc_info_t info, size_t got_address)
{
	//If the sizes don't match then something is wrong
	if(info.entry_size != sizeof(ELF_Rela))
		return -10;

	//Loop through the table
	ELF_Rela* entry = info.address;
	while((size_t)entry - (size_t)info.address < info.size)
	{
		//Get vars out of the entry
		uint32_t* address = RESOLVE_MEM_ADDRESS(libinfo, entry->offset);
		uint8_t type = ELF32_R_TYPE(entry->info);
		size_t index = ELF32_R_SYM(entry->info);

		//get compile time address out of according symbol table entry
		uint32_t compile_time_address;
		uint32_t load_time_address;
		if(index)
		{
			compile_time_address = libinfo->symbol_table_base[index].value;
			
			//Test if this entry needs another lib
			if (compile_time_address == 0)
			{
				//VERIFY: ADDEND USE
				dynamic_linker_add_dynamic_linking(address, libinfo->symbol_table_base[index].name, type, libinfo);
				entry++;
				continue;
			}

			load_time_address = RESOLVE_MEM_ADDRESS(libinfo, compile_time_address);
		}
	
		//Handling depends on the type
		switch (type)
		{
			case R_386_32:
				*address = load_time_address + entry->addend;
				break;
			case R_386_PC32:
				*address = load_time_address + entry->addend - (uint32_t)(address);
				break;
			case R_386_JMP_SLOT:
			case R_386_GLOB_DAT:
				*address = load_time_address;
				break;
			case R_386_RELATIVE:
				*address = libinfo->base_address + entry->addend;
				break;
			case R_386_GOTOFF:
				*address = load_time_address + entry->addend - got_address;
				break;
			case R_386_GOTPC:
				*address = got_address + entry->addend - (uint32_t)(address);
				break;
			case R_386_GOT32:
			case R_386_PLT32:
			case R_386_COPY:
			default:
				return -20;

			case R_386_NONE:
				break;
		}

		entry++;
	}
}


//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------
int process_relocation(libinfo_t* libinfo, dynamic_linker_reloc_info_t info, size_t got_address)
{
	return
		info.type == RELOC_REL
		? process_relocation_rel(libinfo, info, got_address)
		:
			info.type == RELOC_RELA
			? process_relocation_rela(libinfo, info, got_address)
			: -1;
}