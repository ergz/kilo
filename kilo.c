#include <unistd.h>
#include <termios.h>

void enable_raw_mode()
{
	struct termios raw;

	tcgetattr(STDIN_FILENO, &raw);

	raw.c_lflag &= ~(ECHO);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() 
{

	// enable_raw_mode();

	char c;
	// read one byte at a time into c
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	return 0;
}