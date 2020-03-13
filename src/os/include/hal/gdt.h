#ifndef _GDT_H
#define _GDT_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define CODE_DESCRIPTOR_RING_0 0x08
#define DATA_DESCRIPTOR_RING_0 0x10
#define GDT_MAX_COUNT		   0x03

#define GDT_DESC_ACCESS_ACCESS		0b00000001 //Don't set
#define GDT_DESC_ACCESS_READWRITE	0b00000010
#define GDT_DESC_ACCESS_EXPANSION	0b00000100
#define GDT_DESC_ACCESS_EXEC_CODE	0b00001000
#define GDT_DESC_ACCESS_CODEDATA	0b00010000
#define GDT_DESC_ACCESS_DPL_RING0	0b00000000
#define GDT_DESC_ACCESS_DPL_RING1	0b00100000
#define GDT_DESC_ACCESS_DPL_RING2	0b01000000
#define GDT_DESC_ACCESS_DPL_RING3	0b01100000
#define GDT_DESC_ACCESS_MEMORY		0b10000000

#define GDT_DESC_FLAGS_32BIT		0b0100
#define GDT_DESC_FLAGS_GRANULARITY	0b1000

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	uint16_t limitLow;
	uint16_t baseLow;
	uint8_t baseMiddle;
	uint8_t access;
	uint8_t limitHigh_granularity;
	uint8_t baseHigh;
} __attribute__((packed)) gdtDescriptor_t;

typedef struct
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdtr_t;

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initGDT(void);


#endif
