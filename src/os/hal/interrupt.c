#include <hal/interrupt.h>
#include <hal/pic.h>
#include <hal/gdt.h>
#include <string.h>

//--------------------------------------------------------------------------
//          Standart Interrupt Handlers
//--------------------------------------------------------------------------
__interrupt_handler static void standart_interrupt_handler(InterruptFrame_t* frame)
{
    //Do nothing
}

//We need 16 handler
__interrupt_handler static void standart_interrupt_request_handler00(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(0); //Send the endOfInterrupt signal to the PIC
}

__interrupt_handler static void standart_interrupt_request_handler01(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(1); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler02(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(2); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler03(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(3); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler04(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(4); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler05(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(5); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler06(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(6); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler07(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(7); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler08(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(8); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler09(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(9); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler10(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(10); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler11(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(11); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler12(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(12); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler13(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(13); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler14(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(14); //Send the endOfInterrupt signal to the PIC
}
__interrupt_handler static void standart_interrupt_request_handler15(InterruptFrame_t* frame)
{
	//Do nothing
    endOfInterrupt(15); //Send the endOfInterrupt signal to the PIC
}
//------------------------------------------------------------------------------------------
//				Local Vars
//------------------------------------------------------------------------------------------
//This array holds the Interrupt Descriptors
static IDTDescriptor_t idtDescriptors[I86_MAX_INTERRUPTS];
//This is the struct being loaded trough lidt
static IDTR_t idtr;

//------------------------------------------------------------------------------------------
//				Private Function
//------------------------------------------------------------------------------------------
static void setIDTR(void){
	//Set fields of IDTR struct
	idtr.base = (uint32_t)&idtDescriptors[0];
	idtr.limit = sizeof(IDTDescriptor_t) * I86_MAX_INTERRUPTS - 1;

	//Load reference to idtr struct to the register
	asm volatile ("lidt (%0)"::"r"(&idtr));
}

IDTDescriptor_t* setInterruptDescriptor(interruptHandler_t handler, int16_t selector, int8_t createFlags, int index)
{
	//Test if index is in our range
	if(index >= I86_MAX_INTERRUPTS)
		return NULL; //If not return error

	//Get reference to refered idt entry
	IDTDescriptor_t* ret = &idtDescriptors[index];

	//Get address of the handler method
	uint32_t address = (uint32_t)&(*handler);

	//Set idt fields
	ret->selector = selector;
	ret->zero = 0;
	ret->offset_1 = 0b1111111111111111 & address;
	ret->offset_2 = address >> 16;
	ret->type_attr = createFlags;

	//Return IDTDescriptor
	return ret;
}

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------
int initIDT(void)
{
	//Save return codes of submethods
	int returnCode = 0;

	//Remap the Programmable Interrupt Controller to our Interrupt sheme
	returnCode += remapPIC();

	//Clear the buffer array
	memset(idtDescriptors, 0, I86_MAX_INTERRUPTS * sizeof(IDTDescriptor_t));

	//Set every interrupt to a standart handler
	for(int i = 0; i < I86_MAX_INTERRUPTS; i++)
		if(i < 32 || i > 47)
			returnCode += setVect(i, (interruptHandler_t)standart_interrupt_handler);
	
	//Set ISRs individualy
	returnCode += setVect(32, (interruptHandler_t)standart_interrupt_request_handler00);
	returnCode += setVect(33, (interruptHandler_t)standart_interrupt_request_handler01);
	returnCode += setVect(34, (interruptHandler_t)standart_interrupt_request_handler02);
	returnCode += setVect(35, (interruptHandler_t)standart_interrupt_request_handler03);
	returnCode += setVect(36, (interruptHandler_t)standart_interrupt_request_handler04);
	returnCode += setVect(37, (interruptHandler_t)standart_interrupt_request_handler05);
	returnCode += setVect(38, (interruptHandler_t)standart_interrupt_request_handler06);
	returnCode += setVect(39, (interruptHandler_t)standart_interrupt_request_handler07);
	returnCode += setVect(40, (interruptHandler_t)standart_interrupt_request_handler08);
	returnCode += setVect(41, (interruptHandler_t)standart_interrupt_request_handler09);
	returnCode += setVect(42, (interruptHandler_t)standart_interrupt_request_handler10);
	returnCode += setVect(43, (interruptHandler_t)standart_interrupt_request_handler11);
	returnCode += setVect(44, (interruptHandler_t)standart_interrupt_request_handler12);
	returnCode += setVect(45, (interruptHandler_t)standart_interrupt_request_handler13);
	returnCode += setVect(46, (interruptHandler_t)standart_interrupt_request_handler14);
	returnCode += setVect(47, (interruptHandler_t)standart_interrupt_request_handler15);

	//If a submethod fails don't update IDTR register because this may breaks the whole system
	if(returnCode != 0)
		return returnCode;
	
	//Update IDTR register
	setIDTR();

	//Report success
	return returnCode;
}

int setVect(uint8_t index, interruptHandler_t handler)
{
	setInterruptDescriptor(
		handler,								//The handler should be as given
		CODE_DESCRIPTOR_RING_0,					//Kernel's code descriptor
		//    32bit ISR    |    Ring 0   | In use
		_32_INTERRUPT_GATE | _RING_0_DPL | _USED,
		index									//id of the interrupt
		);

	//Report success
	return 0;
}

interruptHandler_t getVect(uint8_t index)
{
	//Create buffer for the handler address and load the pointer to the IDTDescriptor
	uint32_t address;
	IDTDescriptor_t* descriptor = &idtDescriptors[index];

	//Get address and save it to the buffer
	address = descriptor->offset_1 + (descriptor->offset_2 << 16);

	//Return address as a function pointer
	return (interruptHandler_t)address;
}

int setInterruptFlag(void)
{
	//Set interrupt flag instruction
	asm volatile ("sti"::);

	//Report success
	return 0;
}

int clearInterruptFlag(void)
{
	//clear interrupt flag instruction
	asm volatile ("cli"::);

	//Report success
	return 0;
}

void genInt(uint8_t id)
{
	asm volatile (
		"movb (genint+1), %0;"
		"genint:;"
		"int $0;"						// above code modifies the 0 to int number to be generated
		::"r"(id));
}
