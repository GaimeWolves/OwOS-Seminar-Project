#include <hal/pic.h>
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int remapPIC(void)
{
//----------------------------IWC1------------------------------------------
	out(PIC1_REG_COMMAND,ICW1_EXPECT_IC4 | ICW1_INITIALIZE);
	out(PIC2_REG_COMMAND,ICW1_EXPECT_IC4 | ICW1_INITIALIZE);
//----------------------------IWC2------------------------------------------
	out(PIC1_REG_DATA,ICW2_MASTER_MAP);
	out(PIC2_REG_DATA,ICW2_SLAVE_MAP);
//----------------------------IWC3------------------------------------------
	out(PIC1_REG_DATA,ICW3_MASTER_LINE2);
	out(PIC2_REG_DATA,ICW3_SLAVE_LINE2);
//----------------------------IWC4------------------------------------------
	out(PIC1_REG_DATA,ICW4_80X86);
	out(PIC2_REG_DATA,ICW4_80X86);
//----------------------------Clear-----------------------------------------
	out(PIC1_REG_DATA,0);
	out(PIC2_REG_DATA,0);
}

int endOfInterrupt(uint16_t irq)
{
	if(irq >= 8)
		__asm__("out dx, al"::"a"(OCW2_END_OF_INTERRUPT),"d"(PIC2_REG_COMMAND));
	__asm__("out dx, al"::"a"(OCW2_END_OF_INTERRUPT),"d"(PIC1_REG_COMMAND));
}
