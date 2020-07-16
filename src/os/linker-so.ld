ENTRY(entry)

SECTIONS
{
	. = 0x100000;
	_start = .;

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

	.dynsym : ALIGN(4)
	{
		*(.dynsym)
	}
	.dynstr : ALIGN(1)
	{
		*(.dynstr)
	}
	.hash : ALIGN(4)
	{
		*(.hash)
	}
	.dynamic : ALIGN(4)
	{
		*(.dynamic)
	}

	_end = .;
}
