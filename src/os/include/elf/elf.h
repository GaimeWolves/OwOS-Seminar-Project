#ifndef _ELF_H
#define _ELF_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <vfs/vfs.h>
#include <stdbool.h>

#include <elf/elf_header.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	ELF_program_header_entry_t* base;
	uint16_t entry_count;
} ELF_program_header_info_t;

typedef struct 
{
	ELF_section_header_entry_t* base;
	uint16_t entry_count;
} ELF_section_header_info_t;

typedef struct 
{
	FILE* file;

	ELF_header_t* header_cache;
	ELF_program_header_info_t* program_header_info_cache;
	ELF_section_header_info_t* section_header_info_cache;
} ELF_FILE;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
bool is_elf_valid(ELF_FILE* file);											//Checks for magic numbers
ELF_header_t* get_elf_header(ELF_FILE* file);							//Get reference to ELF header struct in file
ELF_program_header_info_t* get_elf_program_header_info(ELF_FILE* file);	//Get info struct of the elf program header of the file
ELF_section_header_info_t* get_elf_section_header_info(ELF_FILE* file);	//Get info struct of the elf section header of the file

void dispose_elf_file_struct(ELF_FILE* file);							//Releases all ELF_FILE related memory
ELF_FILE* create_elf_file_struct(FILE* file);							//Creates a ELF_FILE struct out of a FILE struct

#endif // _ELF_DYNAMIC_H
