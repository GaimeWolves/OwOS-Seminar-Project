#ifndef _STDIO_H
#define _STDIO_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <stddef.h>
#include <stdarg.h>

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

int sprintf(char *buffer, const char *format, ...);
int snprintf(char *buffer, size_t bufsz, const char *format, ...);

int vsprintf(char *buffer, const char *format, va_list ap);
int vsnprintf(char *buffer, size_t bufsz, const char *format, va_list ap);

#endif //_STDIO_H
