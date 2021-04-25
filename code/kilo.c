#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
/*** DEFINES ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION "0.0.1"


/*** DATA ***/

struct editor_config {
	int cursor_x, cursor_y;
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

	if (c == '\x1b') {
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '[') {
			switch(seq[1]) {
				case 'A': {
					return 'w';
				}
				case 'B': {
					return 's';
				}
				case 'C': {
					return 'd';
				}
				case 'D': {
					return 'a';
				}
				
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

void buffer_append(struct abuf *ab, const char *s, int len) 
{
	// void *realloc( void *ptr, size_t new_size );
	char *new  = realloc(ab->b, ab->len + len);

	if (new == NULL) return;

	// copy into new the string s at the adress after the length of 
	// what was already in there
	// void* memcpy( void *dest, const void *src, size_t count );
	memcpy(&new[ab->len], s, len);

	// update the ab struct to new mem and length
	ab->b = new;
	ab->len += len;
}

void buffer_free(struct abuf *ab) 
{
	free(ab->b);
}

/*** INPUT ***/

void editor_move_cursor(char key) 
{
	switch(key) {
		case 'a': {
			E.cursor_x--;
			break;
		}
		case 'd': {
			E.cursor_x++;
			break;
		}
		case 'w': {
			E.cursor_y--;
			break;
		}
		case 's': {
			E.cursor_y++;
			break;
		}
	}
}

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
		case 'w':
		case 's':
		case 'a':
		case 'd': {
			editor_move_cursor(c);
			break;
		}
	}
}

/*** OUTPUT ***/

void editor_draw_rows(struct abuf *ab) 
{
	int y;
	for (y = 0; y < E.screenrows; y++) {

		// go down 1/3 of the screen to print the message out
		if (y == E.screenrows / 3) {
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome), 
				"Kilo editor --version %s", KILO_VERSION);

			if (welcomelen > E.screencols) welcomelen = E.screencols;
			int padding = (E.screencols - welcomelen) / 2;
			if (padding) {
				buffer_append(ab, "~", 1);
				padding--;
			}
			while (padding--) buffer_append(ab, " ", 1);
			buffer_append(ab, welcome, welcomelen);
		} else {
			buffer_append(ab, "~", 1);
		}

		buffer_append(ab, "\x1b[K", 3);
		if (y < E.screenrows - 1) {
			buffer_append(ab, "\r\n", 2);
		}
	}

}

void editor_refresh_screen() 
{
	struct abuf ab = ABUF_INIT;

	buffer_append(&ab, "\x1b[?25l", 6);
	buffer_append(&ab, "\x1b[H", 3);

	editor_draw_rows(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cursor_y+1, E.cursor_x+1);
	buffer_append(&ab, buf, strlen(buf));

	buffer_append(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	buffer_free(&ab);
}


/*** INIT ***/

void init_editor() 
{
	E.cursor_x = 0;
	E.cursor_y = 0;


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