#include <elf/elf.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <memory/heap.h>

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
//				Private function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------
//Checks for magic numbers
bool is_elf_valid(ELF_FILE* file)
{
	ELF_header_t* header = get_elf_header(file);

	return 	header 
			&& header->magic_number == H_MAGIC_NUMBER
			&& header->data == HD_LITTLE_ENDIAN
			&& header->architecture == HA_386
			&& header->header_version == HV_CURRENT
			&& header->elf_class == HC_32
			&& header->version == HV_CURRENT
			&& (header->type == HT_EXEC || header->type == HT_DYN);
}
//Get reference to ELF header struct in file
ELF_header_t* get_elf_header(ELF_FILE* file)
{
	//If header is cached return the cache
	if(file->header_cache)
		return file->header_cache;

	//Get a buffer
	file->header_cache = kmalloc(sizeof(ELF_header_t));

	//Go to start of the file
	vfsSeek(file->file, 0, SEEK_SET);
	//Read the actual bytes
	size_t read = vfsRead(file->file, file->header_cache, sizeof(ELF_header_t));

	//If not exactly sizeof(ELF_header_t) bytes are read an error occured
	if(read != sizeof(ELF_header_t))
	{
		kfree(file->header_cache);
		file->header_cache = NULL;
	}

	return file->header_cache;
}
//Get info struct of the elf program header of the file
ELF_program_header_info_t* get_elf_program_header_info(ELF_FILE* file)
{
	//Test if already cached
	if(file->program_header_info_cache)
		return file->program_header_info_cache;

	//Get header and test if it is not NULL
	ELF_header_t* header = get_elf_header(file);
	if(!header)
		return NULL;

	//Get table related data
	uint32_t table_pos = header->program_header_table_pos;
	uint16_t table_entry_size = header->program_header_table_size;
	uint16_t table_entry_count = header->program_header_table_entry_count;

	size_t table_size = (size_t)table_entry_count * (size_t)table_entry_size;

	//If table_entry size is not as big as our struct something is wrong
	if(table_entry_size != sizeof(ELF_program_header_entry_t))
		return NULL;

	//Get buffer for the table
	ELF_program_header_entry_t* buffer = kmalloc(table_size);

	//Read table into buffer
	vfsSeek(file->file, table_pos, SEEK_SET);
	vfsRead(file->file, buffer, table_size);

	//Create program table info struct
	ELF_program_header_info_t* table_info = kmalloc(sizeof(ELF_program_header_info_t));
	table_info->base = buffer;
	table_info->entry_count = table_entry_count;

	//Set cache
	file->program_header_info_cache = table_info;

	//Return info struct
	return table_info;
}
//Get info struct of the elf section header of the file
ELF_section_header_info_t* get_elf_section_header_info(ELF_FILE* file)
{
	//Test if already cached
	if(file->section_header_info_cache)
		return file->section_header_info_cache;

	//Get header and test if it is not NULL
	ELF_header_t* header = get_elf_header(file);
	if(!header)
		return NULL;

	//Get table related data
	uint32_t table_pos = header->section_header_table_pos;
	uint16_t table_entry_size = header->section_header_table_size;
	uint16_t table_entry_count = header->section_header_table_entry_count;

	size_t table_size = (size_t)table_entry_count * (size_t)table_entry_size;

	//If table_entry size is not as big as our struct something is wrong
	if(table_entry_size != sizeof(ELF_section_header_entry_t))
		return NULL;

	//Get buffer for the table
	ELF_section_header_entry_t* buffer = kmalloc(table_size);

	//Read table into buffer
	vfsSeek(file->file, table_pos, SEEK_SET);
	vfsRead(file->file, buffer, table_size);

	//Create program table info struct
	ELF_section_header_info_t* table_info = kmalloc(sizeof(ELF_section_header_info_t));
	table_info->base = buffer;
	table_info->entry_count = table_entry_count;

	//Set cache
	file->section_header_info_cache = table_info;

	//Return info struct
	return table_info;
}

//Releases all ELF_FILE related memory
void dispose_elf_file_struct(ELF_FILE* file)
{
	//Close file itself
	vfsClose(file->file);

	//If header is cached free it
	if(file->header_cache)
	{
		kfree(file->header_cache);
		file->header_cache = NULL;
	}

	//If program header is cached free it
	if(file->program_header_info_cache)
	{
		kfree(file->program_header_info_cache->base);
		kfree(file->program_header_info_cache);
		file->program_header_info_cache = NULL;
	}

	//If section header is cached free it
	if(file->section_header_info_cache)
	{
		kfree(file->section_header_info_cache->base);
		kfree(file->section_header_info_cache);
		file->section_header_info_cache = NULL;
	}

	//Free ELF_FILE
	kfree(file);
}

//Creates a ELF_FILE struct out of a FILE struct
//If the file is invalid it will be closed
ELF_FILE* create_elf_file_struct(FILE* file)
{
	//If file is NULL return NULL
	if(!file)
		return NULL;

	//Create struct on heap
	ELF_FILE* elf_file = kzalloc(sizeof(ELF_FILE));
	elf_file->file = file;

	//Test if the file is valid
	if(!is_elf_valid(elf_file))
	{
		dispose_elf_file_struct(elf_file);
		elf_file = NULL;
	}

	//Return the pointer to the struct
	return elf_file;
}