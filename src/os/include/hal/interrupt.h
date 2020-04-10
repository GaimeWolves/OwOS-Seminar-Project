#ifndef _INTERRUPT_H
#define _INTERRUPT_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define _32_TASK_GATE       0b00000101
#define _16_INTERRUPT_GATE  0b00000110
#define _16_TRAP_GATE       0b00000111
#define _32_INTERRUPT_GATE  0b00001110
#define _32_TRAP_GATE       0b00001111

#define _STORAGE_SEGMENT    0b00010000

#define _RING_3_DPL         0b01100000
#define _RING_2_DPL         0b01000000
#define _RING_1_DPL         0b00100000
#define _RING_0_DPL         0b00000000

#define _USED               0b10000000
#define _UNUSED             0b00000000

#define I86_MAX_INTERRUPTS  256

#define __interrupt_handler __attribute__((interrupt))
//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
typedef struct
{
	//Length of IDT - 1
	uint16_t limit;

	//Start of the IDT
	uint32_t base;
} __attribute__((packed)) IDTR_t;

typedef struct
{
	uint16_t	offset_1;  //offset bit 0..15
	uint16_t	selector;  //a code descriptor in GDT or LDT
	uint8_t		zero;      //set to 0
	uint8_t		type_attr; //type and attribute
	uint16_t	offset_2;  //offset bit 16..31
} __attribute__((packed)) IDTDescriptor_t;

typedef struct
{
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
} __attribute__((packed)) InterruptFrame_t;


typedef void (__interrupt_handler *interruptHandler_t)(InterruptFrame_t*);

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initIDT(void);

int setVect(uint8_t, interruptHandler_t);
interruptHandler_t *getVect(uint8_t);

int setInterruptFlag(void);
int clearInterruptFlag(void);

void genInt(uint8_t id);
#endif