#include <hal/pic.h>
#include <hal/cpu.h>
#include <stdint.h>

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int remapPIC(void)
{
//----------------------------IWC1------------------------------------------
	outb(PIC1_REG_COMMAND,ICW1_EXPECT_IC4 | ICW1_INITIALIZE);
	outb(PIC2_REG_COMMAND,ICW1_EXPECT_IC4 | ICW1_INITIALIZE);
//----------------------------IWC2------------------------------------------
	outb(PIC1_REG_DATA,ICW2_MASTER_MAP);
	outb(PIC2_REG_DATA,ICW2_SLAVE_MAP);
//----------------------------IWC3------------------------------------------
	outb(PIC1_REG_DATA,ICW3_MASTER_LINE2);
	outb(PIC2_REG_DATA,ICW3_SLAVE_LINE2);
//----------------------------IWC4------------------------------------------
	outb(PIC1_REG_DATA,ICW4_80X86);
	outb(PIC2_REG_DATA,ICW4_80X86);
//----------------------------Clear-----------------------------------------
	outb(PIC1_REG_DATA,0);
	outb(PIC2_REG_DATA,0);

	return 0;
}

int endOfInterrupt(uint16_t irq)
{
	if(irq >= 8)
		outb(PIC2_REG_COMMAND,OCW2_END_OF_INTERRUPT);
	outb(PIC1_REG_COMMAND,OCW2_END_OF_INTERRUPT);

	return 0;
}
