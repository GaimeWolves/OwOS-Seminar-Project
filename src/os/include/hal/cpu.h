#ifndef _CPU_H
#define _CPU_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdint.h>

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
//				Public Function
//------------------------------------------------------------------------------------------
void halt(void);
char* vendorName(void);

inline void outb(uint16_t port, uint8_t data);
inline void outw(uint16_t port, uint16_t data);
inline void outd(uint16_t port, uint32_t data);
#endif
