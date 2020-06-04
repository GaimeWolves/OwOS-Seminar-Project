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
//Linked list type for dynamic section's NEEDED entries
typedef struct dynamic_linker_needed_type dynamic_linker_needed_type_t;
struct dynamic_linker_needed_type
{
	dynamic_linker_needed_type_t* prev;
	size_t name_offset;
};
//Linked list type for symbols needing to be linked dynamically
typedef struct dynamic_linker_dynamic_symbol dynamic_linker_dynamic_symbol_t;
struct dynamic_linker_dynamic_symbol
{
	dynamic_linker_dynamic_symbol_t* prev;
	char* name;
	uint8_t type;
	uint32_t* address;
	uint32_t* addend;
};
//Describes attributes of a relocation section
typedef struct
{
	void* address;
	size_t size;
	size_t entry_size;
	uint8_t type;
} dynamic_linker_reloc_info_t;
//Describes attributes of a library
typedef struct
{
	//Basic information about the library
	ELF_FILE* file;
	void* base_address;
	size_t page_count;

	//Dependancies of the library
	char* libs[MAX_LOADED_LIBS];
	size_t lib_count;

	//Mantatory dynamic section entries
	char* string_table_base;
	size_t string_table_size;
	ELF_symbol_table_entry_t* symbol_table_base;
	size_t symbol_table_entry_size;
	void* hash_table_base;

	//Optimal dynamic section entries
	dynamic_linker_needed_type_t* last_needed_entry;
	dynamic_linker_reloc_info_t reloc_info;

	void* plt_got_address;
	dynamic_linker_reloc_info_t plt_got_info;

	dynamic_linker_dynamic_symbol_t* last_dynamic_entry;
} libinfo_t;

//------------------------------------------------------------------------------------------
//				Macros
//------------------------------------------------------------------------------------------

//Resolves a compile time address to the load time address
#define RESOLVE_MEM_ADDRESS(i,a) (i->base_address + a)
//Get the char* to the library name
#define LIBINFO_NAME(i) i->file->file->file_desc->name
//Load a part of a lib into buffer_pointer.
//Gets buffer from the heap
#define READ_FROM_DISK(libinfo, buffer_pointer, offset, size) \
		buffer_pointer = kmalloc(size); \
		vfsSeek(libinfo->file->file, offset, SEEK_SET); \
		vfsRead(libinfo->file->file, (void*)buffer_pointer, size);

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------
//Save libinfos of all loaded libs
libinfo_t* loaded_libs[MAX_LOADED_LIBS];
size_t loaded_lib_count;

//The dynamic linked shell streams
characterStream_t* in_stream_var;
characterStream_t* out_stream_var;
characterStream_t* err_stream_var;

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

//Load the executable specified by executable an initialisze it with the given streams and the command line args
int linker_main(characterStream_t* in_stream, characterStream_t* out_stream, characterStream_t* err_stream, FILE* executable, int argc, char *argv[]);
//Processes the program header of a library
int process_program_header(libinfo_t* libinfo);

#endif
