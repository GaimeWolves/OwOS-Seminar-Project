ENTRY(entry)

SECTIONS
{
	. = 0x100000;

	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
		_edata = .;
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	_stack_size = 0x4000;
	.stack : ALIGN(4K)
	{
		_stack = .;
		*(.stack)
		. = _stack_size;
		_estack = .;
	}

	_end = .;
}
