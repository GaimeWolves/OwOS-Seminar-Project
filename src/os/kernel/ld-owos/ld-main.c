#include <ld-owos/ld-owos.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <elf/elf.h>

#include <memory/pmm.h>
#include <memory/heap.h>

#include <vfs/vfs.h>

#include <string.h>
#include <stdbool.h>

#include <attribute_defs.h>

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
//Handles the INTERP entry in the program headers
//Returns true if we can handle it
static bool test_demanded_linker(libinfo_t* libinfo, ELF_program_header_entry_t* entry)
{
	//Returncode
	bool returnCode = true;
	char* buffer;

	//Load the specified file offset 
	READ_FROM_DISK(libinfo, buffer, entry->offset, entry->segment_size)

	//Compare the loaded string with our name
	if(strcmp(buffer, "ld-owos") != 0)
		returnCode = false;
	
	//Free buffer
	kfree(buffer);

	return returnCode;
}
//Calculates the count of the needed PMM pages and allocates them
//Sets the fields of the libinfo struct accordingly
static int getPages(libinfo_t* libinfo)
{
	//Get the lowest and the highest program header load address
	size_t minAddress = (size_t)-1;
	size_t maxAddress = 0;

	ELF_program_header_info_t* header = get_elf_program_header_info(libinfo->file);
	for(uint16_t i = 0; i < header->entry_count; i++)
	{
		ELF_program_header_entry_t* current_header = &header->base[i];
		if(current_header->type != PHT_LOAD)
			continue;
		
		if(minAddress == (size_t)-1)
			//If not set then the min address is at the start aligned by the value in alignment
			minAddress = current_header->vaddr - (current_header->vaddr % current_header->alignment);

		if(maxAddress <= current_header->vaddr)
		{
			//Align by alignment
			maxAddress = current_header->vaddr - (current_header->vaddr % current_header->alignment) + current_header->alignment;
		}
	}

	//Calculate needed memory
	size_t address_space_byte_count = maxAddress - minAddress;
	size_t pages = address_space_byte_count / PMM_BLOCK_SIZE;

	//Allocate it
	void* base = pmmAllocContinuous(pages);

	//Set vars
	libinfo->base_address = base;
	libinfo->page_count = pages;

	return base ? 0 : -1;
}
//Checks if the library is position independant
static bool test_pie(ELF_section_header_info_t* header)
{
	//FIXME: IMPLEMENT
	return true;
}
//Checks if the library is already loaded
static bool test_already_loaded(libinfo_t* libinfo)
{
	for(size_t i = 0; i < loaded_lib_count; i++)
	{
		if(strcmp(LIBINFO_NAME(loaded_libs[i]), LIBINFO_NAME(libinfo)) == 0)
			return true;
	}
	return false;
}

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------

//Load the executable specified by executable an initialisze it with the given streams and the command line args
//EXCEPTIONS:
//	-1000: Could not parse executable file
int linker_main(characterStream_t* in_stream, characterStream_t* out_stream, characterStream_t* err_stream, FILE* executable, int argc, char *argv[])
{
	//Set stream vars
	in_stream_var = in_stream;
	out_stream_var = out_stream;
	err_stream_var = err_stream;

	//Extend the file to an ELF file
	ELF_FILE* file = create_elf_file_struct(executable);
	if(!file)
		return -1000;

	//Extend the ELF_FILE to an library
	libinfo_t* libinfo = kzalloc(sizeof(libinfo_t));
	libinfo->file = file;

	//Process program headers and if it returns with 0 execute the program with the given command line args
	int returnCode;
	if(!(returnCode = process_program_header(libinfo)))
	{
		int (*program_entry)(int argc, char *argv[]);
		program_entry = get_elf_header(file)->entry_point + libinfo->base_address;

		returnCode = program_entry(argc, argv);
	}

	//Free memory
	for(size_t i = loaded_lib_count - 1; i > 0; i--)
	{
		
		if(strcmp(LIBINFO_NAME(loaded_libs[i]), "libkernel.so") == 0)
		{
			kfree(loaded_libs[i]->string_table_base);
			kfree(loaded_libs[i]->symbol_table_base);
		}
		else
			pmmFreeContinuous(loaded_libs[i]->base_address, loaded_libs[i]->page_count);

		//Free libinfo
		LIBINFO_FREE(loaded_libs[i])
	}
	loaded_lib_count = 0;

	//return the return code of the application or if the initialization failed of process_program_header
	return returnCode;
}
//Processes the program header of a library
//EXCEPTIONS:
//	- 1: Executable is not position independant
//	- 2: Could not get memory from the PMM
//	- 3: Wrong dynamic linker
//	- 4: Too many shared libraries
//	-10: process_dynamic_section
int process_program_header(libinfo_t* libinfo)
{
	//Test if we loaded this lib already
	if(test_already_loaded(libinfo))
	{
		//Free the allocated memory
		LIBINFO_FREE(libinfo)
		return 0;
	}

	//We can't use not-pie executables
	if(!test_pie(get_elf_section_header_info(libinfo->file)))
	{
		//Free the allocated memory
		LIBINFO_FREE(libinfo)
		return -1;
	}

	//Something in page allocation failed
	if(getPages(libinfo))
	{
		//Free the allocated memory
		LIBINFO_FREE(libinfo)
		return -2;
	}

	//Dynamic section program header entry
	ELF_program_header_entry_t* needLinking = NULL;

	//Process each program header entry
	ELF_program_header_info_t* header = get_elf_program_header_info(libinfo->file);
	for(uint16_t i = 0; i < header->entry_count; i++)
	{
		ELF_program_header_entry_t* current_header = &header->base[i];

		switch(current_header->type)
		{
			case PHT_LOAD:
				//Load from the specified offset
				vfsSeek(libinfo->file->file, current_header->offset, SEEK_SET);
				//Load specified count to our buffer gotten by getPages
				vfsRead(libinfo->file->file, (void*)((size_t)current_header->vaddr + libinfo->base_address), current_header->segment_size);
				//Fill rest of buffer with 0
				uint32_t not_in_file_space = current_header->memory_size - current_header->segment_size;
				void* not_in_file_space_in_memory_start = (void*)(current_header->segment_size + current_header->vaddr + libinfo->base_address);
				memset(not_in_file_space_in_memory_start, 0, not_in_file_space);
				break;
			case PHT_DYNAMIC:
				//Save the header of the dynamic section
				needLinking = current_header;
				break;
			case PHT_INTERP:
				//Only meaningful for exec files
				if(get_elf_header(libinfo->file)->type != HT_EXEC)
					break;
				//Wrong linker
				if(!test_demanded_linker(libinfo, current_header))
				{
					//Free the allocated memory
					LIBINFO_FREE(libinfo)
					return -3;
				}
				break;
			case PHT_NOTE:
			case PHT_SHLIB:
			case PHT_PHDR:
			case PHT_NULL:
				//No action needed
				break;
			default:
				//Unknown type
				return -4;
		}
	}

	if(loaded_lib_count == MAX_LOADED_LIBS)
	{
		//FIXME: HANDLE OUT OF ARRAY SPACE
		LIBINFO_FREE(libinfo)
		return -4;
	}

	loaded_libs[loaded_lib_count++] = libinfo;

	//If there was a dynamic section entry process the dynamic section
	if(needLinking)
	{
		int returnCode;
		if(returnCode = process_dynamic_section(libinfo, needLinking))
			return returnCode - 10;
	}

	return 0;
}