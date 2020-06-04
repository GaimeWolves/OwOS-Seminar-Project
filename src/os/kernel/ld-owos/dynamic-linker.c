#include "dynamic-linker.h"

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <ld-owos/ld-owos.h>

#include <elf/elf_dynamic_section.h>
#include <elf/elf_relocation.h>

#include <memory/heap.h>

#include <vfs/vfs.h>

#include <string.h>
#include <stdbool.h>

#include <attribute_defs.h>

#include "relocation.h"

//------------------------------------------------------------------------------------------
//				Macro
//------------------------------------------------------------------------------------------
#define IS_DYNAMIC_NULL_ENTRY(e) (e->type == DST_NULL) //FIXME: ADD FULL SPECIFICATION

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Private function
//------------------------------------------------------------------------------------------

//Test if all mandatory fields are set
//true if all are set otherwise false
static __ALWAYS_INLINE__ bool check_vars(libinfo_t* libinfo)
{
	return
		libinfo->string_table_base != NULL
		&& libinfo->string_table_size != 0
		&& libinfo->symbol_table_base != NULL
		&& libinfo->symbol_table_entry_size != 0
		&& libinfo->hash_table_base != NULL
		? true
		: false;
}

//Handles NEEDED entries of the dynamic section
//EXCEPTIONS:
//	-1: No such file
//	-2: File is not an ELF file
//	-3: Too many loaded libraries
static int handle_needed_libs(char* name)
{
	//Process filename
	//Search lib in root dir
	size_t len = strlen(name);
	char* file_name = kzalloc(len + 2);
	file_name[0] = '/';
	strcpy(&file_name[1], name);
	//Open file
	FILE* file = vfsOpen(file_name, "r");
	//Free memory
	kfree(file_name);

	//If we don't have a valid file descriptor the library don't exist
	if(!file)
		return -1;

	//Extend it to an ELF_FILE
	ELF_FILE* elf_file = create_elf_file_struct(file);

	//If we don't have a valid ELF_FILE descriptor the file is not an ELF file
	if(!elf_file)
	{
		//Close the file descriptor
		vfsClose(file);
		return -2;
	}

	//Get an libinfo struct
	libinfo_t* libinfo = kzalloc(sizeof(libinfo_t));
	libinfo->file = elf_file;

	//Special handling for kernel linkage
	if(strcmp(name, "libkernel.so") == 0)
	{
		//Test if already loaded
		for(size_t i = 0; i < loaded_lib_count; i++)
		{
			if(strcmp(LIBINFO_NAME(loaded_libs[i]), LIBINFO_NAME(libinfo)) == 0)
			{
				//Free memory
				LIBINFO_FREE(libinfo)
				return 0;
			}
		}

		//We can only have MAX_LOADED_LIBS libraries
		if(loaded_lib_count == MAX_LOADED_LIBS)
			return -3;

		//Add as loaded lib
		loaded_libs[loaded_lib_count++] = libinfo;

		//Get DYNSYM and DYNSTR section out of the section header
		size_t dynsym_file_offset, dynsym_file_size, dynstr_file_offset, dynstr_file_size;
		ELF_section_header_info_t* header = get_elf_section_header_info(libinfo->file);
		ELF_header_t* elf_header = get_elf_header(libinfo->file);
		ELF_section_header_entry_t* section_string_table_entry = &header->base[elf_header->section_header_table_name_index];	//String table containing the section names
		char* buffer;

		//First load the string table
		READ_FROM_DISK(libinfo, buffer, section_string_table_entry->offset, section_string_table_entry->size)

		//Search through the section header
		for(uint16_t i = 0; i < header->entry_count && (dynsym_file_offset != 0 || dynstr_file_offset != 0); i++)
		{
			ELF_section_header_entry_t* current_header = &header->base[i];

			switch(current_header->type)
			{
				//There should be only one such section
				case SHT_DYNSYM:
					dynsym_file_offset = current_header->offset;
					dynsym_file_size = current_header->size;
					break;
				//There are multiple sections with this type
				case SHT_STRTAB:
					//Test if we found the searched one
					if(strcmp(&buffer[current_header->name], ".dynstr") == 0)
					{
						dynstr_file_offset = current_header->offset;
						dynstr_file_size = current_header->size;
					}
					break;
				default:
					break;
			}
		}

		//The string table is no longer needed so free the buffer
		kfree(buffer);

		//Load DYNSYM on heap and set libinfo fields accordingly
		READ_FROM_DISK(libinfo, buffer, dynsym_file_offset, dynsym_file_size)
		libinfo->symbol_table_base = buffer;
		libinfo->symbol_table_entry_size = sizeof(ELF_symbol_table_entry_t);
		libinfo->page_count = dynsym_file_size;

		//Load DYNSTR on heap and set libinfo accordingly
		READ_FROM_DISK(libinfo, buffer, dynstr_file_offset, dynstr_file_size)
		libinfo->string_table_base = buffer;
		libinfo->string_table_size = dynstr_file_size;

		return 0;
	}

	//Handle as generic library
	return process_program_header(libinfo);
}
//Resolves dynamic linked symbols of a library
//EXCEPTIONS:
//	-1: Could not find section in section table
//	-2: Could not resolve symbol address
//	-3: Could not handle entry type
int resolve_symbols(libinfo_t* libinfo)
{
	//Loop until every entry is resolved
	while(libinfo->last_dynamic_entry)
	{
		//Get symbol attributes
		char* name = libinfo->last_dynamic_entry->name;
		uint32_t* address = libinfo->last_dynamic_entry->address;
		uint8_t type = libinfo->last_dynamic_entry->type;
		uint32_t addend = libinfo->last_dynamic_entry->addend;

		uint32_t load_time_address = 0;
		//Search symbol in libs
		for(size_t i = 0; i < loaded_lib_count && load_time_address == 0; i++)
		{
			//get lib entry
			libinfo_t* lib = loaded_libs[i];
			//Test if we need depend on this lib
			bool needed = false;
			for(size_t dep_lib_index = 0; dep_lib_index < libinfo->lib_count; dep_lib_index++)
			{
				char* dep_lib_name = libinfo->libs[dep_lib_index];
				if(strcmp(LIBINFO_NAME(lib), dep_lib_name) == 0)
				{
					needed = true;
					break;
				}
			}
			//If not just skip it
			if(!needed)
				continue;

			//Get symbol table entry count
			size_t symbol_table_entry_count;
			//Handle libkernel.so separate
			if(strcmp(LIBINFO_NAME(lib), "libkernel.so"))
			{
				//If it is not the kernel library just look in the section header table
				ELF_section_header_entry_t* symbol_table_section_entry;
				ELF_section_header_info_t* section_header_info = get_elf_section_header_info(lib->file);
				for(size_t j = 0; j < section_header_info->entry_count; j++)
				{
					ELF_section_header_entry_t* entry = &section_header_info->base[j];
					//Test if this section is the searched by testing the memory address
					if(RESOLVE_MEM_ADDRESS(lib, entry->addr) == lib->symbol_table_base)
					{
						symbol_table_section_entry = entry;
						break;
					}
				}
				if(!symbol_table_section_entry)
					return -1;

				symbol_table_entry_count = symbol_table_section_entry->size / symbol_table_section_entry->entry_size;
			}
			else
			{
				//If it is the kernel library the total size is in page_count
				symbol_table_entry_count = lib->page_count / lib->symbol_table_entry_size;
			}
			

			//Loop through the symbol table
			for(size_t j = 0; j < symbol_table_entry_count; j++)
			{
				ELF_symbol_table_entry_t* entry = &lib->symbol_table_base[j];
				char* entry_name = &lib->string_table_base[entry->name];

				//Test if the current symbol is the searched
				if(strcmp(entry_name, name) == 0)
				{
					load_time_address = RESOLVE_MEM_ADDRESS(lib, entry->value);
					break;
				}
			}
		}
		
		//If the symbol is not resolved something went wrong
		if(!load_time_address)
			return -2;
		
		//Handle each type
		switch (type)
		{
			case R_386_32:
				*address = load_time_address + addend;
				break;
			case R_386_PC32:
				*address = load_time_address + addend - (uint32_t)(address);
				break;
			case R_386_JMP_SLOT:
			case R_386_GLOB_DAT:
				*address = load_time_address;
				break;
			case R_386_RELATIVE:
			case R_386_GOTOFF:
			case R_386_GOTPC:
			case R_386_GOT32:
			case R_386_PLT32:
			case R_386_COPY:
			default:
				return -3;

			case R_386_NONE:
				break;
		}

		//Go to next entry
		dynamic_linker_dynamic_symbol_t* next = libinfo->last_dynamic_entry->prev;
		kfree(libinfo->last_dynamic_entry);
		libinfo->last_dynamic_entry = next;
	}
}

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------

//Process the dynamic section of a library
//EXCEPTIONS:
//	- 1: Could not handle needed dynamic entry type
//	- 2: Could not get needed dynamic section entries
//	-10: process_relocation 
//	-20: resolve_symbols
int process_dynamic_section(libinfo_t* libinfo, ELF_program_header_entry_t* entry)
{
	int returnCode;

	//Get the dynamic section address in memory
	ELF_dynamic_section_entry_t* dyn_entry = (ELF_dynamic_section_entry_t*)RESOLVE_MEM_ADDRESS(libinfo, entry->vaddr);

	//Iterate through the table
	while(!IS_DYNAMIC_NULL_ENTRY(dyn_entry))
	{
		//Handling depends on the type
		switch (dyn_entry->type)
		{
			case DST_NEEDED:;
				//Setup the struct and set it as the last entry
				dynamic_linker_needed_type_t* needed = kmalloc(sizeof(dynamic_linker_needed_type_t));
				needed->name_offset = dyn_entry->value.val;
				needed->prev = libinfo->last_needed_entry;
				libinfo->last_needed_entry = needed;
				break;
			case DST_HASH:
				//Set the according var
				libinfo->hash_table_base = RESOLVE_MEM_ADDRESS(libinfo, dyn_entry->value.ptr);
				break;
			case DST_STRTAB:
				//Set the according var
				libinfo->string_table_base = RESOLVE_MEM_ADDRESS(libinfo, dyn_entry->value.ptr);
				break;
			case DST_SYMTAB:
				//Set the according var
				libinfo->symbol_table_base = RESOLVE_MEM_ADDRESS(libinfo, dyn_entry->value.ptr);
				break;
			case DST_STRSZ:
				//Set the according var
				libinfo->string_table_size = dyn_entry->value.val;
				break;
			case DST_SYMENT:
				//Set the according var
				libinfo->symbol_table_entry_size = dyn_entry->value.val;
				break;

			case DST_RELA:
				//Set the according var
				libinfo->reloc_info.type = RELOC_RELA;
				libinfo->reloc_info.address = RESOLVE_MEM_ADDRESS(libinfo, dyn_entry->value.ptr);
				break;
			case DST_REL:
				//Set the according var
				libinfo->reloc_info.type = RELOC_REL;
				libinfo->reloc_info.address = RESOLVE_MEM_ADDRESS(libinfo, dyn_entry->value.ptr);
				break;
			case DST_RELASZ:
			case DST_RELSZ:
				//Set the according var
				libinfo->reloc_info.size = dyn_entry->value.val;
				break;
			case DST_RELAENT:
			case DST_RELENT:
				//Set the according var
				libinfo->reloc_info.entry_size = dyn_entry->value.val;
				break;

			case DST_JMPREL:
				//Set the according var
				libinfo->plt_got_info.address = RESOLVE_MEM_ADDRESS(libinfo, dyn_entry->value.ptr);
				break;
			case DST_PLTRELSZ:
				//Set the according var
				libinfo->plt_got_info.size = dyn_entry->value.val;
				break;
			case DST_PLTGOT:
				//Set the according var
				libinfo->plt_got_address = dyn_entry->value.ptr;
				break;
			case DST_PLTREL:
				//Set the according var
				libinfo->plt_got_info.type = 
					dyn_entry->value.val == DST_REL
					? RELOC_REL
					: 
						dyn_entry->value.val == DST_RELA
						? RELOC_RELA
						: RELOC_NULL;
				break;

			case DST_INIT:
			case DST_FINI:
				//Can't handle this needed section
				return -1;
			case DST_TEXTREL:
			case DST_SYMBOLIC:
			case DST_DEBUG:
			case DST_SONAME:
			case DST_RPATH:
			case DST_NULL:
			default:
				break;
		}

		dyn_entry++;
	}
	//Check mandatory vars
	if(!check_vars(libinfo))
		return -2;

	//First process needed libs
	if(libinfo->last_needed_entry)
	{
		dynamic_linker_needed_type_t* current_entry = libinfo->last_needed_entry;
		while(current_entry)
		{
			//Get the name of the needed lib
			char* name_address = &libinfo->string_table_base[current_entry->name_offset];
			//Handle the lib
			if(returnCode = handle_needed_libs(name_address))
				return returnCode;

			//Add to lib dependencies
			libinfo->libs[libinfo->lib_count++] = name_address;

			//Move to next entry
			current_entry = current_entry->prev;
			kfree(libinfo->last_needed_entry);
			libinfo->last_needed_entry = current_entry;
		}
	}

	//Process general relocation
	if(libinfo->reloc_info.type != RELOC_NULL)
		returnCode = process_relocation(libinfo, libinfo->reloc_info);
	if(returnCode)
		return returnCode - 10;

	//Process plt relocation
	if(libinfo->plt_got_info.type != RELOC_NULL)
	{
		libinfo->plt_got_info.entry_size = libinfo->plt_got_info.type == RELOC_REL ? sizeof(ELF_Rel) : libinfo->plt_got_info.type == RELOC_RELA ? sizeof(ELF_Rela) : 0;
		process_relocation(libinfo, libinfo->plt_got_info);
	}
	if(returnCode)
		return returnCode - 10;

	//Resolve symbols
	returnCode = resolve_symbols(libinfo);

	return returnCode ? returnCode - 20 : 0;
}
//Add an entry to the needed dynamic linked symbol linklist
void dynamic_linker_add_dynamic_linking(libinfo_t* libinfo, uint32_t* address, size_t name_index, uint8_t type, uint32_t addend)
{
	//Get new entry
	dynamic_linker_dynamic_symbol_t* symbol = kmalloc(sizeof(dynamic_linker_dynamic_symbol_t));

	//Set fields
	symbol->prev = libinfo->last_dynamic_entry;
	symbol->name = &libinfo->string_table_base[name_index];
	symbol->type = type;
	symbol->address = address;
	symbol->addend = addend;

	//Update last entry
	libinfo->last_dynamic_entry = symbol;
}