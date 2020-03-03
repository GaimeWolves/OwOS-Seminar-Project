#ifndef _STDINT_H
#define _STDINT_H
//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

typedef char				int8_t;
typedef unsigned char		uint8_t;
typedef short				int16_t;
typedef unsigned short		uint16_t;
typedef int					int32_t;
typedef unsigned int		uint32_t;
typedef long long			int64_t;
typedef unsigned long long	uint64_t;

typedef int				intptr_t;
typedef unsigned int	uintptr_t;

#define INT8_MIN	-128
#define INT16_MIN	-32768
#define INT32_MIN	-2147483648
#define INT64_MIN	-9223372036854775808LL

#define INT8_MAX	127
#define INT16_MAX	32767
#define INT32_MAX	2147483647
#define INT64_MAX	9223372036854775807LL

#define UINT8_MAX	0xff
#define UINT16_MAX	0xffff
#define UINT32_MAX	0xffffffff
#define UINT64_MAX	0xffffffffffffffffULL

#define INTPTR_MIN	INT32_MIN
#define INTPTR_MAX	INT32_MAX
#define UINTPTR_MAX	UINT32_MAX

#define INT8_C(val) ((int8_t) + (val))
#define UINT8_C(val) ((uint8_t) + (val##U))
#define INT16_C(val) ((int16_t) + (val))
#define UINT16_C(val) ((uint16_t) + (val##U))

#define INT32_C(val) val##L
#define UINT32_C(val) val##UL
#define INT64_C(val) val##LL
#define UINT64_C(val) val##ULL

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

#endif //_STDINT_H
