#include <unistd.h>

int main() 
{
	char c;
	// read one byte at a time into c
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	return 0;
}