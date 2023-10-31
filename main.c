#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h> // for sleeping
#include <signal.h> // for catching Ctrl+C to free memory before terminating program

#define CLEAR_SCREEN() printf("%c2J", '\x9b')
#define DOUBLE_WIDTH() printf("\e#6")
#define HIDE_CURSOR() printf("\x9b?25l")
#define SHOW_CURSOR() printf("\x9b?25h")

#define UP() printf("\x8c")
#define DOWN() printf("\n")
#define LEFT() printf("\b")
#define UPx2() printf("\x8c\x8c")
#define DOWNx2() printf("\n\n")
#define LEFTx2() printf("\b\b")
#define UPxN(n) printf("\x9b%dA", n)
#define DOWNxN(n) printf("\x9b%dB", n)
#define LEFTxN(n) printf("\x9b%dD", n)
#define RIGHTxN(n) printf("\x9b%dC", n)

#define TAB() printf("\t")
#define TABx2() printf("\t\t")
#define TABx3() printf("\t\t\t")

#define MOVE_TO(i, j) printf("\x9b%d;%dH", i+1, j+1)
#define MOVE_TO_START_OF_CURRENT_LINE() printf("\r")
#define MOVE_TO_START_OF_LINE(i) printf("\x9b%dH", i+1)
#define MOVE_TO_FIRST_LINE_AT_POSITION(j) printf("\x9b;%dH", j+1)
#define MOVE_TO_ORIGIN() printf("\x9bH")

#define USE_INTERNATIONAL_CHARS() printf("\e)<\e~")
#define USE_BLOCK_CHARS() printf("\e)@\e~")
#define LOAD_BLOCK_CHARS() printf("\eP1;1;1{@????????/????????;^^^^^???/????????;?????^^^/????????;^^^^^^^^/????????;_____???/~~~~~???;~~~~~???/~~~~~???;_____^^^/~~~~~???;~~~~~^^^/~~~~~???;?????___/?????~~~;^^^^^___/?????~~~;?????~~~/?????~~~;^^^^^~~~/?????~~~;________/~~~~~~~~;~~~~~___/~~~~~~~~;_____~~~/~~~~~~~~;~~~~~~~~/~~~~~~~~\e\\\e)@\e~")

#define FLUSH() fflush(stdout)

#define PRINT_BORDER() printf("========================================")

#define ROW_LOOP for (int row = 0; row < ROWS; row++)
#define COLUMN_LOOP for (int col = 0; col < COLUMNS; col++)
#define LINE_LOOP for (int line = 2; line < HEIGHT-1; line++)
#define POSITION_LOOP for (int position = 0; position < WIDTH; position++)

#define BLANK 161

#define HEIGHT 24
#define WIDTH 40

#define ROWS 42
#define COLUMNS 80

bool **alive, **aux, **initial;
char **screen;
int current_line, current_position;
int OFFSETS[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

void shift_x(int dx) {
	if (dx == 0) return;

	if (dx == 1) printf("%c", screen[current_line][current_position]);
	else if (dx == 2) printf("%.2s", screen[current_line] + current_position);
	else if (dx >= 3) RIGHTxN(dx);
	
	else if (dx == -1) LEFT();
	else if (dx == -2) LEFTx2();
	else LEFTxN(-dx);
}

void shift_y(int dy) {
	if (dy == 0) return;

	if (dy == 1) DOWN();
	else if (dy == 2) DOWNx2();
	else if (dy >= 3) DOWNxN(dy);
	
	else if (dy == -1) UP();
	else if (dy == -2) UPx2();
	else UPxN(-dy);
}

void update_cells() {
	ROW_LOOP {
		COLUMN_LOOP {
			int neighbors = 0;
			for (int i = 0; i < 8; i++) {
				int dr = OFFSETS[i][0], dc = OFFSETS[i][1];
				neighbors += alive[(ROWS+row+dr)%ROWS][(COLUMNS+col+dc)%COLUMNS];
			}
			if (alive[row][col]) aux[row][col] = (neighbors == 2 || neighbors == 3);
			else aux[row][col] = (neighbors == 3);
		}
	}

	bool **swapper = alive;
	alive = aux;
	aux = swapper;
}

void update_screen(bool print_screen) {
	LINE_LOOP {
		int row = 2*line - 4;
		POSITION_LOOP {
			int col = 2*position;
			char new_char = (char) (BLANK + alive[row][col] + 2*alive[row][col+1] + 4*alive[row+1][col] + 8*alive[row+1][col+1]);
			if (print_screen) {
				if (new_char != screen[line][position]) {
					int dx = position - current_position;
					int dy = line - current_line;
					if (dx == 0) shift_y(dy);
					else if (dy == 0) shift_x(dx);
					else MOVE_TO(line, position);
					printf("%c", new_char);
				}
			}
			screen[line][position] = new_char;
		}
	}
	FLUSH();
}

bool ** alloc_grid() {
	bool **grid = (bool**) malloc(ROWS * sizeof(bool*));
	ROW_LOOP {
		grid[row] = (bool*) calloc(COLUMNS, sizeof(bool));
	}
	return grid;
}

void free_grid(bool **grid) {
	ROW_LOOP {
		free(grid[row]);	
	}
	free(grid);
}

void interrupt_handler(int a) {
	free_grid(alive);
	free_grid(aux);
	free_grid(initial);

	LINE_LOOP {
		free(screen[line]);
	}
	free(screen);

	CLEAR_SCREEN();
	MOVE_TO_ORIGIN();
	SHOW_CURSOR();
	exit(0);
}

int main() {
	// To handle interruptions through Ctrl+C and exit gracefully
	static struct sigaction sigact = { 0 };
	sigact.sa_handler = interrupt_handler;
	sigaction(SIGINT, &sigact, NULL);

	alive = alloc_grid();
	aux = alloc_grid();
	initial = alloc_grid();

	screen = (char**) malloc(HEIGHT * sizeof(char*));
	LINE_LOOP {
		screen[line] = (char*) malloc(WIDTH * sizeof(char));
		POSITION_LOOP {
			screen[line][position] = BLANK;
		}
	}

	FILE *file = fopen("initial_conway.txt", "r");
	char file_line[100];
	ROW_LOOP {
		if (!fgets(file_line, 100, file)) break;
		COLUMN_LOOP {
			if (file_line[col] == '\0') {
				break;
			}
			if (file_line[col] == 'X') {
				alive[row][col] = true;
				initial[row][col] = true;
			}
		}
	}
	fclose(file);

	CLEAR_SCREEN();
	HIDE_CURSOR();
	MOVE_TO_ORIGIN();
	USE_INTERNATIONAL_CHARS();
	char title[80];
	sprintf(title, "[CONWAY'S GAME OF LIFE]   Francisco Manr%cquez (2023)", 16*14 + 13);
	int pad = 40 - strlen(title)/2;
	printf("%*s%s%*s", pad, "", title, pad, "");
	USE_BLOCK_CHARS();
	for (int line = 1; line < HEIGHT; line++) { 
		DOWN();
		DOUBLE_WIDTH();
	}
	MOVE_TO_START_OF_LINE(1);
	PRINT_BORDER();
	MOVE_TO_START_OF_LINE(WIDTH-1);
	PRINT_BORDER();
	
	struct timespec sleep_time, remaining;
	sleep_time.tv_sec = 0;
	sleep_time.tv_nsec = 0.125e9;
	
	// Code
	while (1) {
		for (int iteration = 0; iteration < 1250; iteration++) {
			update_screen(true);
			update_cells();
			nanosleep(&sleep_time, &remaining);
		}

		ROW_LOOP {
			COLUMN_LOOP {
				alive[row][col] = initial[row][col];
			}
		}
	
		update_screen(true);
	}
	
	return 0;
}
