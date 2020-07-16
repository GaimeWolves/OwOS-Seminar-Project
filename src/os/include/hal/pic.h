#ifndef _PIC_H
#define _PIC_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>
//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------
#define PIC1_REG_COMMAND	0x20
#define PIC1_REG_STATUS		0x20
#define PIC1_REG_DATA		0x21
#define PIC1_REG_IMR		0x21

#define PIC2_REG_COMMAND	0xA0
#define PIC2_REG_STATUS		0xA0
#define PIC2_REG_DATA		0xA1
#define PIC2_REG_IMR		0xA1

#define ICW1_EXPECT_IC4		0b00000001
#define ICW1_NO_SLAVE		0b00000010
#define ICW1_TRIGGERED_MODE	0b00001000
#define ICW1_INITIALIZE		0b00010000

#define ICW2_MASTER_MAP		0x20
#define ICW2_SLAVE_MAP		0x28

#define ICW3_MASTER_LINE0	0b00000001
#define ICW3_MASTER_LINE1	0b00000010
#define ICW3_MASTER_LINE2	0b00000100
#define ICW3_MASTER_LINE3	0b00001000
#define ICW3_MASTER_LINE4	0b00010000
#define ICW3_MASTER_LINE5	0b00100000
#define ICW3_MASTER_LINE6	0b01000000
#define ICW3_MASTER_LINE7	0b10000000

#define ICW3_SLAVE_LINE0	0b000
#define ICW3_SLAVE_LINE1	0b001
#define ICW3_SLAVE_LINE2	0b010
#define ICW3_SLAVE_LINE3	0b011
#define ICW3_SLAVE_LINE4	0b100
#define ICW3_SLAVE_LINE5	0b101
#define ICW3_SLAVE_LINE6	0b110
#define ICW3_SLAVE_LINE7	0b111

#define ICW4_80X86			0b00000001
#define ICW4_AUTO_EOI		0b00000010
#define ICW4_BUFFER_MASTER	0b00000100
#define ICW4_BUFFERED		0b00001000
#define ICW4_SPECIAL_FULLY_NESTED_MODE 0b00010000

#define OCW2_END_OF_INTERRUPT	0b00100000

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int remapPIC(void);
int endOfInterrupt(uint16_t);

#endif