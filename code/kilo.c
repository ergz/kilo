#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
/*** DEFINES ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** DATA ***/

struct editor_config {
	int screenrows;
	int screencols;
	struct termios orig_termios;

};

struct editor_config E;


/*** TERMINAL ***/

void die(const char *s) 
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

void disable_raw_mode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
		die("tcsetattr");
	}
}

int term_get_window_size(int *rows, int *cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
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
	// store the startup attributes in the E.orig_termios
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disable_raw_mode);

	struct termios raw = E.orig_termios;

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

char editor_read_key() 
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

struct abuf {
	char *b;
	int len;
}

#define ABUF_INIT {NULL, 0}

/*** INPUT ***/
void editor_process_keypress() 
{
	char c = editor_read_key();

	switch(c) {
		case CTRL_KEY('q'): {
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		}
	}
}

/*** OUTPUT ***/

void editor_draw_rows() 
{
	int y;
	for (y = 0; y < E.screenrows; y++) {
		write(STDOUT_FILENO, "~", 1);

		if (y < E.screenrows - 1) {
			write(STDOUT_FILENO, "\r\n", 2);
		}
	}

}

void editor_refresh_screen() 
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	editor_draw_rows();

	write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** INIT ***/

void init_editor() 
{
	if (term_get_window_size(&E.screenrows, &E.screencols) == -1) die("term_get_window_size");
}

int main() 
{

	enable_raw_mode();
	init_editor();

	while (1) {
		editor_refresh_screen();
		editor_process_keypress();


		write(STDOUT_FILENO, "\x1b[H", 3);
	}

	return 0;
}