#include <hal/gdt.h>
#include <string.h>

//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
//This array holds the gdts
static gdtDescriptor_t gdt_descriptors[GDT_MAX_COUNT];
//This is the struct being loaded trough lgdt
static gdtr_t gdtr;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static inline void setGDTR(void)
{
	//Set fields of gdtr struct
	gdtr.base = (uint32_t)&gdt_descriptors;
	gdtr.limit = sizeof(gdtDescriptor_t) * GDT_MAX_COUNT - 1;

	//Load reference to gdtr struct to the register
	asm volatile ("lgdt (%0)":: "r" (&gdtr));
}

static int setGDTDescriptor(int32_t base, int32_t limit, int8_t access, int8_t flags, size_t index)
{
	//Test if index is in our range
	if(index >= GDT_MAX_COUNT)
		return -1; //If not return error
	
	//Get reference to refered gdt entry
	gdtDescriptor_t* descriptor = &gdt_descriptors[index];

	//Set gdt fields
	descriptor->limitLow = limit & 0xFFFF;
	descriptor->limitHigh_granularity |= (limit >> 16) & 0xF;
	descriptor->baseLow = base & 0xFFFF;
	descriptor->baseMiddle = (base >> 16) & 0xFF;
	descriptor->baseHigh = (base >> 24) & 0xFF;
	descriptor->access = access;
	descriptor->limitHigh_granularity |= (flags & 0xF) << 4;

	//Report success
	return 0;
}

static int resetGDTDescriptor(size_t index)
{
	//Test if index is in our range
	if(index >= GDT_MAX_COUNT)
		return -1; //If not return error
	
	//Zero the entry out
	memset(&gdt_descriptors[index], 0, sizeof(gdtDescriptor_t));

	//Report success
	return 0;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initGDT(void)
{
	//Save return codes of submethods
	int returnCode = 0;

	//Set null descriptor
	returnCode += resetGDTDescriptor(
		//1st descriptor
		0
		);
	//Set code descriptor
	returnCode += setGDTDescriptor(
		0,			//This descriptor starts at address 0
		0xFFFFFF,	//This descriptor ends at address 0xFFFFFF
					//=> occupies full address space
		//Read access allowed     | Execution allowed         | Signals code/data segment| Segment uses ring level 0 | Signals valid selector
		GDT_DESC_ACCESS_READWRITE | GDT_DESC_ACCESS_EXEC_CODE | GDT_DESC_ACCESS_CODEDATA | GDT_DESC_ACCESS_DPL_RING0 | GDT_DESC_ACCESS_MEMORY,
		//Signals 32bits     | Limit is 4kiB aligned (is shifted up 8 times)
		GDT_DESC_FLAGS_32BIT | GDT_DESC_FLAGS_GRANULARITY,
		//2nd descriptor
		1
		);
	//Set data descriptor
	returnCode += setGDTDescriptor(
		0,			//This descriptor starts at address 0
		0xFFFFFF,	//This descriptor ends at address 0xFFFFFF 
					//=> occupies full address space (21 address lines: 2²¹ < 0xFFFFFF)
		//Read access allowed     | Signals code/data segment| Segment uses ring level 0 | Signals valid selector
		GDT_DESC_ACCESS_READWRITE | GDT_DESC_ACCESS_CODEDATA | GDT_DESC_ACCESS_DPL_RING0 | GDT_DESC_ACCESS_MEMORY,
		//Signals 32bits     | Limit is 4kiB aligned (is shifted up 8 times)
		GDT_DESC_FLAGS_32BIT | GDT_DESC_FLAGS_GRANULARITY,
		//3rd selector
		2
		);

	//If a submethod fails don't update GDTR register because this may breaks the whole system
	if(returnCode != 0)
		return returnCode;

	//Update GDTR register
	setGDTR();

	//Report success
	return 0;
}
