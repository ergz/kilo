#include <unistd.h>
#include <termios.h>
#include <stdlib.h>


// create a struct for the original termios, we will want to reset 
// the terminal to this state once we are done
struct termios orig_termios;

void disable_raw_mode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/*
Enable Raw Mode
raw mode will make the terminal be such that is appropriate for text editing.
The changes that are made to the terminal are:
* NO ECHO

*/
void enable_raw_mode()
{
	// store the startup attributes in the orig_termios
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disable_raw_mode);

	struct termios raw = orig_termios;

	tcgetattr(STDIN_FILENO, &raw);

	raw.c_lflag &= ~(ECHO);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() 
{

	enable_raw_mode();

	char c;
	// read one byte at a time into c
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	return 0;
}