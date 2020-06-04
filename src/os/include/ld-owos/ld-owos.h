#ifndef _LD_OWOS_H
#define _LD_OWOS_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stream/stream.h>
#include <vfs/vfs.h>
#include <elf/elf.h>
#include <elf/elf_symbol_table.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define MAX_LOADED_LIBS	10

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct dynamic_linker_needed_type dynamic_linker_needed_type_t;
typedef struct dynamic_linker_dynamic_symbol dynamic_linker_dynamic_symbol_t;

struct dynamic_linker_needed_type
{
	dynamic_linker_needed_type_t* prev;
	size_t name_offset;
};

struct dynamic_linker_dynamic_symbol
{
	dynamic_linker_dynamic_symbol_t* prev;
	char* name;
	uint8_t type;
	uint32_t* address;
};

typedef struct
{
	void* address;
	size_t size;
	size_t entry_size;
	uint8_t type;
} dynamic_linker_reloc_info_t;

typedef struct
{
	ELF_FILE* file;
	void* base_address;
	size_t page_count;

	char* libs[MAX_LOADED_LIBS];
	size_t lib_count;

	//Mantatory
	char* string_table_base;
	size_t string_table_size;
	ELF_symbol_table_entry_t* symbol_table_base;
	size_t symbol_table_entry_size;
	void* hash_table_base;

	//Optimal
	dynamic_linker_needed_type_t* dynamic_linker_last_needed_entry;
	dynamic_linker_reloc_info_t dynamic_linker_reloc_info;

	void* dynamic_linker_plt_got_address;
	dynamic_linker_reloc_info_t dynamic_linker_plt_got_info;

	dynamic_linker_dynamic_symbol_t* dynamic_linker_last_dynamic_entry;
} libinfo_t;

//------------------------------------------------------------------------------------------
//				Macros
//------------------------------------------------------------------------------------------
#define RESOLVE_MEM_ADDRESS(i,a) (i->base_address + a)
#define LIBINFO_NAME(i) i->file->file->file_desc->name

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------
libinfo_t* loaded_libs[MAX_LOADED_LIBS];
size_t loaded_lib_count;

characterStream_t* in_stream_var;
characterStream_t* out_stream_var;
characterStream_t* err_stream_var;

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int linker_main(characterStream_t* in_stream, characterStream_t* out_stream, characterStream_t* err_stream, FILE* executable, int argc, char *argv[]);
int process_program_header(libinfo_t* libinfo);

#endif
