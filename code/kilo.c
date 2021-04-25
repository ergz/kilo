#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/*** DATA ***/

// create a struct for the original termios, we will want to reset 
// the terminal to this state once we are done
struct termios orig_termios;

/*** TERMINAL ***/

void die(const char *s) 
{
	perror(s);
	exit(1);
}

void disable_raw_mode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
		die("tcsetattr");
	}
}

/*
Enable Raw Mode
raw mode will make the terminal be such that is appropriate for text editing.
The changes that are made to the terminal are:
* NO ECHO - dont echo what i type in
* NO CANONICAL - this means we will be reading byte by byte
* NO SIGSTOP - ignores the ctrl-c sig stop key
* NO XON

*/
void enable_raw_mode()
{
	// store the startup attributes in the orig_termios
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	atexit(disable_raw_mode);

	struct termios raw = orig_termios;

	tcgetattr(STDIN_FILENO, &raw);

	// local flags
	raw.c_lflag &= ~(ECHO|ICANON|ISIG|IEXTEN);

	// input flags
	raw.c_iflag &= ~(IXON|ICRNL|BRKINT|INPCK|ISTRIP);

	// output flags
	raw.c_oflag &= ~(OPOST);

	raw.c_cflag |= (CS8);

	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

/*** INIT ***/

int main() 
{

	enable_raw_mode();

	while (1) {
		char c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d '(%c)'\r\n", c, c);
		}

		if (c == 'q') break;
	}

	return 0;
}