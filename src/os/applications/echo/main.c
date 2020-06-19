#include <stdio.h>

int main(int argc, char* argv[])
{
	for(int i = 1; i < argc; i++)
	{
		printf(argv[i]);
		if(i < argc - 1)
			putchar(' ');
	}

	return 0;
}