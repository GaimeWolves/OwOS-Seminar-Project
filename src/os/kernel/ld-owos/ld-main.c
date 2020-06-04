#include <ld-owos/ld-owos.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <elf/elf.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <vfs/vfs.h>
#include <string.h>

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
int test_demanded_linker(ELF_program_header_entry_t* entry, FILE* file_descriptor)
{
	//Returncode
	int returnCode = 0;

	//Get buffer of the specified size
	char* buffer = kmalloc(entry->memory_size);

	//Load from the specified offset
	vfsSeek(file_descriptor, entry->offset, SEEK_SET);
	//Load specified count to our buffer
	vfsRead(file_descriptor, buffer, entry->segment_size);

	//Compare with our name
	if(!strcmp(buffer, "ld-owos"))
		returnCode = -1;
	
	//Free buffer
	kfree(buffer);

	return returnCode;
}

int getPages(libinfo_t* libinfo)
{
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

	size_t address_space_byte_count = maxAddress - minAddress;
	size_t pages = address_space_byte_count / PMM_BLOCK_SIZE;

	void* base = pmmAllocContinuous(pages);

	libinfo->base_address = base;
	libinfo->page_count = pages;

	return 0;
}

bool test_pie(ELF_section_header_info_t* header)
{
	//FIXME: IMPLEMENT
	return true;
}

bool test_already_loaded(libinfo_t* libinfo)
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
int linker_main(characterStream_t* in_stream, characterStream_t* out_stream, characterStream_t* err_stream, FILE* executable, int argc, char *argv[])
{
	//Set stream vars
	in_stream_var = in_stream;
	out_stream_var = out_stream;
	err_stream_var = err_stream;

	ELF_FILE* file = create_elf_file_struct(executable);
	if(!file)
		return -1000;

	libinfo_t* libinfo = kzalloc(sizeof(libinfo_t));
	libinfo->file = file;

	int returnCode;
	if(!process_program_header(libinfo))
	{
		int (*program_entry)(int argc, char *argv[]);
		program_entry = get_elf_header(file)->entry_point + libinfo->base_address;

		returnCode = program_entry(argc, argv);
	}
	//FIXME: FREE MEMORY

	return returnCode;
}

int process_program_header(libinfo_t* libinfo)
{
	//Test if we loaded this lib already
	if(test_already_loaded(libinfo))
	{
		//FIXME: FREE LIBINFO
		return 0;
	}

	//We can't use not pie executables
	if(!test_pie(get_elf_section_header_info(libinfo->file)))
		return -1;

	//Something in page allocation failed
	if(getPages(libinfo))
		return -2;

	ELF_program_header_entry_t* needLinking = NULL;

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
				needLinking = current_header;
				break;
			case PHT_INTERP:
				//Only meaningful for exec files
				if(get_elf_header(libinfo->file)->type != HT_EXEC)
					break;
				//Wrong linker
				if(!test_demanded_linker(current_header, libinfo->file->file))
					return -3;
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
	}

	loaded_libs[loaded_lib_count++] = libinfo;

	if(needLinking)
	{
		if(process_dynamic_section(libinfo, needLinking))
			return -10;
	}

	return 0;
}