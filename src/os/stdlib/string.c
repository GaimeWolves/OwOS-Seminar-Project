#include <string.h>

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------
#include <stdbool.h>

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
//				Private function
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public function
//------------------------------------------------------------------------------------------

size_t strlen(const char *s)
{
	int d0;
	size_t res;
	asm volatile("repne\n\t"
		"scasb"
		: "=c" (res), "=&D" (d0)
		: "1" (s), "a" (0), "0" (0xffffffffu)
		: "memory");
	return ~res - 1;
}
size_t strnlen(const char *s, size_t count)
{
	int d0;
	int res;
	asm volatile("movl %2,%0\n\t"
		"jmp 2f\n"
		"1:\tcmpb $0,(%0)\n\t"
		"je 3f\n\t"
		"incl %0\n"
		"2:\tdecl %1\n\t"
		"cmpl $-1,%1\n\t"
		"jne 1b\n"
		"3:\tsubl %2,%0"
		: "=a" (res), "=&d" (d0)
		: "c" (s), "1" (count)
		: "memory");
	return res;
}
int strcmp(const char *cs, const char *ct)
{
	int d0, d1;
	int res;
	asm volatile("1:\tlodsb\n\t"
		"scasb\n\t"
		"jne 2f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"xorl %%eax,%%eax\n\t"
		"jmp 3f\n"
		"2:\tsbbl %%eax,%%eax\n\t"
		"orb $1,%%al\n"
		"3:"
		: "=a" (res), "=&S" (d0), "=&D" (d1)
		: "1" (cs), "2" (ct)
		: "memory");
	return res;
}
int strncmp(const char *cs, const char *ct, size_t count)
{
	int res;
	int d0, d1, d2;
	asm volatile("1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"scasb\n\t"
		"jne 3f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %%eax,%%eax\n\t"
		"jmp 4f\n"
		"3:\tsbbl %%eax,%%eax\n\t"
		"orb $1,%%al\n"
		"4:"
		: "=a" (res), "=&S" (d0), "=&D" (d1), "=&c" (d2)
		: "1" (cs), "2" (ct), "3" (count)
		: "memory");
	return res;
}
char *strcpy(char *dest, const char *src)
{
	int d0, d1, d2;
	asm volatile("1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		: "=&S" (d0), "=&D" (d1), "=&a" (d2)
		: "0" (src), "1" (dest) : "memory");
	return dest;
}
char *strncpy(char *dest, const char *src, size_t count)
{
	int d0, d1, d2, d3;
	asm volatile("1:\tdecl %2\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"rep\n\t"
		"stosb\n"
		"2:"
		: "=&S" (d0), "=&D" (d1), "=&c" (d2), "=&a" (d3)
		: "0" (src), "1" (dest), "2" (count) : "memory");
	return dest;
}
char *strcat(char *dest, const char *src)
{
	int d0, d1, d2, d3;
	asm volatile("repne\n\t"
		"scasb\n\t"
		"decl %1\n"
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		: "=&S" (d0), "=&D" (d1), "=&a" (d2), "=&c" (d3)
		: "0" (src), "1" (dest), "2" (0), "3" (0xffffffffu) : "memory");
	return dest;
}
char *strncat(char *dest, const char *src, size_t count)
{
	int d0, d1, d2, d3;
	asm volatile("repne\n\t"
		"scasb\n\t"
		"decl %1\n\t"
		"movl %8,%3\n"
		"1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %2,%2\n\t"
		"stosb"
		: "=&S" (d0), "=&D" (d1), "=&a" (d2), "=&c" (d3)
		: "0" (src), "1" (dest), "2" (0), "3" (0xffffffffu), "g" (count)
		: "memory");
	return dest;
}
char *strstr(const char *cs, const char *ct)
{
	int	d0, d1;
	register char *__res;
	__asm__ __volatile__(
		"movl %6,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
		"movl %%ecx,%%edx\n"
		"1:\tmovl %6,%%edi\n\t"
		"movl %%esi,%%eax\n\t"
		"movl %%edx,%%ecx\n\t"
		"repe\n\t"
		"cmpsb\n\t"
		"je 2f\n\t"		/* also works for empty string, see above */
		"xchgl %%eax,%%esi\n\t"
		"incl %%esi\n\t"
		"cmpb $0,-1(%%eax)\n\t"
		"jne 1b\n\t"
		"xorl %%eax,%%eax\n\t"
		"2:"
		: "=a" (__res), "=&c" (d0), "=&S" (d1)
		: "0" (0), "1" (0xffffffff), "2" (cs), "g" (ct)
		: "dx", "di");
	return __res;
}

int memcmp(const void *s1, const void *s2, size_t len)
{
	int diff = 0;
	asm("repe; cmpsb; setl %%al; setg %%cl; ror $1 ,%%eax; addl %%ecx, %%eax" 
	    : "+a" (diff), "+D" (s1), "+S" (s2), "+c" (len));
	return diff;
}
void* memcpy(void *to, const void *from, size_t n)
{
	int d0, d1, d2;
	asm volatile("rep ; movsl\n\t"
		     "movl %4,%%ecx\n\t"
		     "andl $3,%%ecx\n\t"
		     "jz 1f\n\t"
		     "rep ; movsb\n\t"
		     "1:"
		     : "=&c" (d0), "=&D" (d1), "=&S" (d2)
		     : "0" (n / 4), "g" (n), "1" ((long)to), "2" ((long)from)
		     : "memory");
	return to;
}
void* memset(void *s, char c, size_t count)
{
	int d0, d1;
	asm volatile("rep\n\t"
		     "stosb"
		     : "=&c" (d0), "=&D" (d1)
		     : "a" (c), "1" (s), "0" (count)
		     : "memory");
	return s;
}
void *memmove(void *dest, const void *src, size_t n)
{
	int d0,d1,d2,d3,d4,d5;
	char *ret = dest;

	__asm__ __volatile__(
		/* Handle more 16bytes in loop */
		"cmp $0x10, %0\n\t"
		"jb	1f\n\t"

		/* Decide forward/backward copy mode */
		"cmp %2, %1\n\t"
		"jb	2f\n\t"

		/*
		 * movs instruction have many startup latency
		 * so we handle small size by general register.
		 */
		"cmp  $680, %0\n\t"
		"jb 3f\n\t"
		/*
		 * movs instruction is only good for aligned case.
		 */
		"mov %1, %3\n\t"
		"xor %2, %3\n\t"
		"and $0xff, %3\n\t"
		"jz 4f\n\t"
		"3:\n\t"
		"sub $0x10, %0\n\t"

		/*
		 * We gobble 16byts forward in each loop.
		 */
		"3:\n\t"
		"sub $0x10, %0\n\t"
		"mov 0*4(%1), %3\n\t"
		"mov 1*4(%1), %4\n\t"
		"mov  %3, 0*4(%2)\n\t"
		"mov  %4, 1*4(%2)\n\t"
		"mov 2*4(%1), %3\n\t"
		"mov 3*4(%1), %4\n\t"
		"mov  %3, 2*4(%2)\n\t"
		"mov  %4, 3*4(%2)\n\t"
		"lea  0x10(%1), %1\n\t"
		"lea  0x10(%2), %2\n\t"
		"jae 3b\n\t"
		"add $0x10, %0\n\t"
		"jmp 1f\n\t"

		/*
		 * Handle data forward by movs.
		 */
		".p2align 4\n\t"
		"4:\n\t"
		"mov -4(%1, %0), %3\n\t"
		"lea -4(%2, %0), %4\n\t"
		"shr $2, %0\n\t"
		"rep movsl\n\t"
		"mov %3, (%4)\n\t"
		"jmp 11f\n\t"
		/*
		 * Handle data backward by movs.
		 */
		".p2align 4\n\t"
		"6:\n\t"
		"mov (%1), %3\n\t"
		"mov %2, %4\n\t"
		"lea -4(%1, %0), %1\n\t"
		"lea -4(%2, %0), %2\n\t"
		"shr $2, %0\n\t"
		"std\n\t"
		"rep movsl\n\t"
		"mov %3,(%4)\n\t"
		"cld\n\t"
		"jmp 11f\n\t"

		/*
		 * Start to prepare for backward copy.
		 */
		".p2align 4\n\t"
		"2:\n\t"
		"cmp  $680, %0\n\t"
		"jb 5f\n\t"
		"mov %1, %3\n\t"
		"xor %2, %3\n\t"
		"and $0xff, %3\n\t"
		"jz 6b\n\t"

		/*
		 * Calculate copy position to tail.
		 */
		"5:\n\t"
		"add %0, %1\n\t"
		"add %0, %2\n\t"
		"sub $0x10, %0\n\t"

		/*
		 * We gobble 16byts backward in each loop.
		 */
		"7:\n\t"
		"sub $0x10, %0\n\t"

		"mov -1*4(%1), %3\n\t"
		"mov -2*4(%1), %4\n\t"
		"mov  %3, -1*4(%2)\n\t"
		"mov  %4, -2*4(%2)\n\t"
		"mov -3*4(%1), %3\n\t"
		"mov -4*4(%1), %4\n\t"
		"mov  %3, -3*4(%2)\n\t"
		"mov  %4, -4*4(%2)\n\t"
		"lea  -0x10(%1), %1\n\t"
		"lea  -0x10(%2), %2\n\t"
		"jae 7b\n\t"
		/*
		 * Calculate copy position to head.
		 */
		"add $0x10, %0\n\t"
		"sub %0, %1\n\t"
		"sub %0, %2\n\t"

		/*
		 * Move data from 8 bytes to 15 bytes.
		 */
		".p2align 4\n\t"
		"1:\n\t"
		"cmp $8, %0\n\t"
		"jb 8f\n\t"
		"mov 0*4(%1), %3\n\t"
		"mov 1*4(%1), %4\n\t"
		"mov -2*4(%1, %0), %5\n\t"
		"mov -1*4(%1, %0), %1\n\t"

		"mov  %3, 0*4(%2)\n\t"
		"mov  %4, 1*4(%2)\n\t"
		"mov  %5, -2*4(%2, %0)\n\t"
		"mov  %1, -1*4(%2, %0)\n\t"
		"jmp 11f\n\t"

		/*
		 * Move data from 4 bytes to 7 bytes.
		 */
		".p2align 4\n\t"
		"8:\n\t"
		"cmp $4, %0\n\t"
		"jb 9f\n\t"
		"mov 0*4(%1), %3\n\t"
		"mov -1*4(%1, %0), %4\n\t"
		"mov  %3, 0*4(%2)\n\t"
		"mov  %4, -1*4(%2, %0)\n\t"
		"jmp 11f\n\t"

		/*
		 * Move data from 2 bytes to 3 bytes.
		 */
		".p2align 4\n\t"
		"9:\n\t"
		"cmp $2, %0\n\t"
		"jb 10f\n\t"
		"movw 0*2(%1), %%dx\n\t"
		"movw -1*2(%1, %0), %%bx\n\t"
		"movw %%dx, 0*2(%2)\n\t"
		"movw %%bx, -1*2(%2, %0)\n\t"
		"jmp 11f\n\t"

		/*
		 * Move data for 1 byte.
		 */
		".p2align 4\n\t"
		"10:\n\t"
		"cmp $1, %0\n\t"
		"jb 11f\n\t"
		"movb (%1), %%cl\n\t"
		"movb %%cl, (%2)\n\t"
		".p2align 4\n\t"
		"11:"
		: "=&c" (d0), "=&S" (d1), "=&D" (d2),
		  "=r" (d3),"=r" (d4), "=r"(d5)
		:"0" (n),
		 "1" (src),
		 "2" (dest)
		:"memory");

	return ret;

}
void *memchr(const void *cs, int c, size_t count)
{
	int d0;
	void *res;
	if (!count)
		return NULL;
	asm volatile("repne\n\t"
		"scasb\n\t"
		"je 1f\n\t"
		"movl $1,%0\n"
		"1:\tdecl %0"
		: "=D" (res), "=&c" (d0)
		: "a" (c), "0" (cs), "1" (count)
		: "memory");
	return res;
}
void *memscan(void *addr, int c, size_t size)
{
	if (!size)
		return addr;
	asm volatile("repnz; scasb\n\t"
	    "jnz 1f\n\t"
	    "dec %%edi\n"
	    "1:"
	    : "=D" (addr), "=c" (size)
	    : "0" (addr), "1" (size), "a" (c)
	    : "memory");
	return addr;
}