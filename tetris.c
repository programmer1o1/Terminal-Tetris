/* 
  TETRIS SOURCE CODE
  THERE IS ONE FOR WINDOWS
  THERE IS ONE FOR UNIX
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h> 
#include <sys/ioctl.h>
#endif

#define BOARD_WIDTH 20
#define BOARD_HEIGHT 21

typedef enum {
    I, J, L, O, S, T, Z, INV_L
} Tetromino;

const char *TETRIS_COLORS[] = {
    "\x1b[47m", // 0
    "\x1b[41m", // 1
    "\x1b[42m", // 2
    "\x1b[43m", // 3
    "\x1b[44m", // 4
    "\x1b[45m", // 5
    "\x1b[46m", // 6
    "\x1b[42m", // 7
    "\e[0;100m", // 8  
    "\x1b[0m"   // 9
};

const int GHOST_COLOR_INDEX = 8;

const char *TETROMINO_SHAPES[][4] = {
    {"...."
     "####"
     "...."
     "....",
     "..#."
     "..#."
     "..#."
     "..#."},
    {"...."
     ".###"
     "..#."
     "....",
     "..#."
     ".##."
     "..#."
     "....",
     "...."
     ".#.."
     "###."
     "....",
     ".#.."
     "##.."
     ".#.."
     "...."},
    {"...."
     "###."
     "#..."
     "....",
     "#..."
     "#..."
     "##.."
     "....",
     "#..."
     "###."
     "...."
     "....",
     ".##."
     "..#."
     "..#."
     "...."},
    {"...."
     "##.."
     "##.."},
    {"...."
     ".##."
     "##.."
     "....",
     "...."
     ".#.."
     ".##."
     "..#."},
    {"...."
     "##.."
     ".##."
     "....",
     "...."
     "..#."
     ".##."
     ".#.."},
    {"...."
     "###."
     "#..."
     "....",
     "#..."
     "##.."
     "#..."
     "....",
     "...."
     "#..."
     "###."
     "....",
     ".#.."
     ".##."
     ".#.."
     "...."},
    {"...."
     "###."
     "..#."
     "....",
     ".#.."
     ".#.."
     "##.."
     "....",
     "...."
     ".#.."
     "###."
     "....",
     "##.."
     "#..."
     "#..."
     "...."}};

const char *INVERTED_L_SHAPES[] = {
    "...."
    "...."
    "###."
    "..#.",

    "...."
    "..#."
    ".##."
    "..#.",

    "...."
    "...."
    ".#.."
    "###.",

    "...."
    ".#.."
    "##.."
    ".#.."};

typedef struct {
    int x, y;
} Point;

typedef struct {
    char board[BOARD_HEIGHT][BOARD_WIDTH];
    Tetromino currentTetromino;
    Tetromino nextTetromino;
    Point currentPositions[4];
    int rotation;
    bool gameOver;
    int score;
    int level;
    int linesCleared;
    bool paused;
    bool showGhost;
    bool toggleColors;
    bool showDots;  
} Tetris;

void drawPausedScreen();
void spawnTetromino(Tetris *tetris);
void draw(const Tetris *tetris);
void drawGhost(const Tetris *tetris);
void drawNextTetromino(Tetromino tetromino, int row, Tetris *tetris);
void input(Tetris *tetris);
void update(Tetris *tetris);
bool tetris_move(Tetris *tetris, int dx, int dy);
void rotate(Tetris *tetris);
bool isValidPosition(const Tetris *tetris, const Point *positions);
void lockTetromino(Tetris *tetris);
void removeFullLines(Tetris *tetris);
void drawGameOverScreen(const Tetris *tetris, int score, int level, int linesCleared);
int _kbhit();
int _getch();

#ifdef _WIN32
#include <signal.h>
void sigint_handler(int sig_num) {
    /* Reset handler (optional) */
    signal(SIGINT, sigint_handler);
    printf("\033[?25h");  // make cursor visible
    fflush(stdout);
    exit(1);
}
#else
#include <signal.h>
#include <termios.h>

struct termios orig_termios; /* Store original terminal settings */

/* Reset terminal settings on SIGINT */
void sigint_handler(int sig_num)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\033[?25h"); /* Make cursor visible */
    fflush(stdout);
    exit(1);
}
#endif 

int main() {

    // On Unix systems, get terminal settings
#ifndef _WIN32
    /* Get current terminal settings */
    tcgetattr(STDIN_FILENO, &orig_termios);

    /* Make necessary changes */
    struct termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
#endif

signal(SIGINT, sigint_handler);  // register the signal handler

#ifdef _WIN32
    system("chcp 65001"); // Set code page to UTF-8, so that the border do not bug in Windows
    system("cls");
#else
    struct termios old_tio, new_tio;

    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO,&old_tio);

    /* we want to keep the old setting to restore them a the end */
    new_tio=old_tio;

    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &=(~ICANON & ~ECHO);

    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
    system("clear");
#endif

    Tetris tetris = {0};
   
    /* Initialize the board cells to -1 */
    // for (int y = 0; y < BOARD_HEIGHT; ++y) {
    //   for (int x = 0; x < BOARD_WIDTH; ++x) {
    //     tetris.board[y][x] = -1;
    //     }
    // }

    // Initialize the board cells to '.'
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            tetris.board[y][x] = '.';
        }
    }

    srand(time(0)); // Initialize random number generator
    tetris.nextTetromino = rand() % 8; 
    // Place '=' character here:
    spawnTetromino(&tetris);
    tetris.level = 1;
    tetris.linesCleared = 0;
    tetris.score = 0;
    tetris.paused = false;
    tetris.showGhost = true;
    tetris.toggleColors = true;  
    tetris.showDots = false; 

    bool game_over_screen_displayed = false;
        bool wasJustPaused = false;
        
while (1) {
        input(&tetris);

        if (tetris.paused) {
            if (!wasJustPaused) {
                drawPausedScreen();
                wasJustPaused = true;
            }
            continue;  //skip everything else if the game is paused
        }

        wasJustPaused = false;

        if (!tetris.gameOver) {
            draw(&tetris);
            update(&tetris);
        } 
        else if (tetris.gameOver && !game_over_screen_displayed) {
            game_over_screen_displayed = true;
            drawGameOverScreen(&tetris, tetris.score, tetris.level, tetris.linesCleared);
            _getch();
        printf(
"####### ######## ######## ########  ####  ######\n"
"  ##    ##          ##    ##     ##  ##  ##    ##\n"
"  ##    ##          ##    ##     ##  ##  ##\n"
"  ##    ######      ##    ########   ##   ######\n"
"  ##    ##          ##    ##   ##    ##        ##\n"
"  ##    ##          ##    ##    ##   ##  ##    ##\n"
"  ##    ########    ##    ##     ## ####  ######\n"
        );
        printf("THANKS FOR PLAYING!!!\n");
        break;
    } else {
    #ifdef _WIN32
        Sleep(1);
    #else
        usleep(50 * 1000);
    #endif
        input(&tetris);
        }
    }
    
    #ifndef _WIN32
       /* restore the former settings */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    #endif
    
    printf("\033[?25h");
    
    return 0;
}

void drawPausedScreen() {
#ifdef _WIN32
    system("cls");
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int horizontal_padding = (csbi.srWindow.Right - csbi.srWindow.Left - BOARD_WIDTH) / 2;
    int vertical_padding = (csbi.srWindow.Bottom - csbi.srWindow.Top - 9) / 2;
#else
    printf("\033[H\033[J"); // On Unix-like systems, we can just print the escape codes directly
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int horizontal_padding = (w.ws_col - BOARD_WIDTH) / 2;
    int vertical_padding = (w.ws_row - 9) / 2;
#endif

    for(int i = 0; i < vertical_padding; i++) printf("\n");

    for(int i = 0; i < horizontal_padding; i++) printf(" ");
    printf("####################\n");

    for(int i = 0; i < horizontal_padding; i++) printf(" ");
    printf("#    GAME PAUSED   #\n");

    for(int i = 0; i < horizontal_padding; i++) printf(" ");
    printf("# Press 'p' to     #\n");

    for(int i = 0; i < horizontal_padding; i++) printf(" ");
    printf("# resume the game  #\n");

    for(int i = 0; i < horizontal_padding; i++) printf(" ");
    printf("####################\n");
}

void removeFullLines(Tetris *tetris) {
    int linesRemoved = 0;

    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        bool isFullLine = true;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (tetris->board[y][x] == '.') {
                isFullLine = false;
                break;
            }
        }

        if (isFullLine) {
            for (int y2 = y; y2 > 0; --y2) {
                memcpy(tetris->board[y2], tetris->board[y2 - 1], BOARD_WIDTH);
            }
            memset(tetris->board[0], '.', BOARD_WIDTH);
            linesRemoved++;
        }
    }

    if (linesRemoved > 0) {
        int lineScore[] = {0, 100, 300, 500, 800};
        int bonus = (linesRemoved - 1) * 100;
        tetris->score += (lineScore[linesRemoved] + bonus) * tetris->level;
        tetris->linesCleared += linesRemoved;

        if (tetris->linesCleared >= tetris->level * 10) {
            tetris->level++;
        }
    }
}

void spawnTetromino(Tetris *tetris) {
    tetris->currentTetromino = tetris->nextTetromino;
    tetris->nextTetromino = (rand() % 8);
    tetris->rotation = 0;

    const char *shape = NULL;
    if (tetris->currentTetromino == INV_L) {
        shape = INVERTED_L_SHAPES[tetris->rotation];
    } else {
        shape = TETROMINO_SHAPES[tetris->currentTetromino][tetris->rotation];
    }

    int idx = 0;
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            if (shape[y * 4 + x] == '#') {
                tetris->currentPositions[idx++] = (Point){x + BOARD_WIDTH / 2 - 2, y};
            }
        }
    }
}

void drawNextTetromino(Tetromino tetromino, int row, Tetris *tetris) {
    const char *shape = TETROMINO_SHAPES[tetromino][0];
    for (int x = 0; x < 4; ++x) {
    if (tetris->toggleColors) {
        if (shape[row * 4 + x] == '#') {
            printf("%s \033[0m", TETRIS_COLORS[tetromino]);
        } else {
            printf(" ");
        }
    } else {
      if (shape[row * 4 + x] == '#') {
        putchar('#');
      } else {
        putchar(' ');
      }
    }
  }
}

uint64_t getCurrentTimeMillis() {
#ifdef _WIN32
    LARGE_INTEGER frequency, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&currentTime);
    return (currentTime.QuadPart * 1000) / frequency.QuadPart;
#else
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    return tspec.tv_sec * 1000 + tspec.tv_nsec / 1000000;
#endif
}

void drawGameOverScreen(const Tetris *tetris, int score, int level, int linesCleared) {
    draw(tetris);

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int horizontal_padding = (csbi.srWindow.Right - csbi.srWindow.Left - BOARD_WIDTH) / 2;
    int vertical_padding = (csbi.srWindow.Bottom - csbi.srWindow.Top - 9) / 2;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int horizontal_padding = (w.ws_col - BOARD_WIDTH) / 2;
    int vertical_padding = (w.ws_row - 9) / 2;
#endif

    printf("\033[%d;%dH", vertical_padding);
	
	#ifdef _WIN32
    const char *gameOverText[] = {
        " +---------------+", // This looks bad, but it has to work anyways right? Because it's a bug for some reason.
        "|   GAME OVER   |",
        "+---------------+",
        "| Score: 000000 |",
        "| Level: 01     |",
        "| Lines: 00     |",
        "+---------------+",
        "| Press any key |",
        "| to exit       |",
        "+---------------+"
    };
	#else
	const char *gameOverText[] = {
        "+---------------+", 
        "|   GAME OVER   |",
        "+---------------+",
        "| Score: 000000 |",
        "| Level: 01     |",
        "| Lines: 00     |",
        "+---------------+",
        "| Press any key |",
        "| to exit       |",
        "+---------------+"
    };
	#endif

    for (int i = 0; i < sizeof(gameOverText) / sizeof(gameOverText[0]); ++i) {
        for (int j = 0; j < horizontal_padding - BOARD_WIDTH; ++j) {
            putchar(' ');
        }

        if (i == 3) {
            printf("| Score: %06d |", score);
        } else if (i == 4) {
            printf("| Level: %02d     |", level);
        } else if (i == 5) {
            printf("| Lines: %02d     |", linesCleared);
        } else {
            printf("%s", gameOverText[i]);
        }
        printf("\033[E"); 
    }
}

void draw(const Tetris *tetris) {
#ifdef _WIN32
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CHAR_INFO consoleBuffer[BOARD_HEIGHT][BOARD_WIDTH];

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            consoleBuffer[y][x].Char.AsciiChar = tetris->board[y][x];
            consoleBuffer[y][x].Attributes = BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_INTENSITY;;
        }
    }
		printf("\033[?25l");
#else
        printf("\033[?25l"); // Hide the cursor in Unix
        printf("\033[H");   // Reset the cursor position to top left.
#endif

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int horizontal_padding = (csbi.srWindow.Right - csbi.srWindow.Left + 1 - BOARD_WIDTH) / 2;
    int vertical_padding = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1 - BOARD_HEIGHT) / 2;
    int window_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int window_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int horizontal_padding = (w.ws_col - (BOARD_WIDTH + 1)) / 2; // Adjust the padding calculation
    int vertical_padding = (w.ws_row - BOARD_HEIGHT) / 2;
    int window_width = w.ws_col;
    int window_height = w.ws_row;
#endif

// Calculate the length of the title string
int titleLength = strlen("T E T R I S");
	
    // Draw top border with corners and title
    printf("\033[%d;%dH\u256D", vertical_padding, horizontal_padding); // Position to the left padding plus vertical padding
    for (int i = 0; i < BOARD_WIDTH; ++i) printf("\u2500");
    printf("\u256E\n");  // Top-right corner
	

// Calculate the position of the title, taking into account its length
int titlePosition = horizontal_padding + (BOARD_WIDTH - titleLength) / 2;

// Print the title on a separate line above the game board
printf("\033[%d;%dH", vertical_padding, titlePosition); // Position for title
printf("T E T R I S\n");

    // Draw the Tetris board and borders
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        printf("\033[%d;%dH\u2502", vertical_padding + y + 1, horizontal_padding); // Left border
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            bool isCurrentTetromino = false;
            bool isGhostTetromino = false;

            if (tetris->showGhost) {
                Point ghostPositions[4];
                memcpy(ghostPositions, tetris->currentPositions, sizeof(Point) * 4);

                while (isValidPosition(tetris, ghostPositions)) {
                    for (int i = 0; i < 4; ++i) {
                        ghostPositions[i].y++;
                    }
                }

                for (int i = 0; i < 4; ++i) {
                    ghostPositions[i].y--;

                    if (ghostPositions[i].x == x && ghostPositions[i].y == y) {
                        isGhostTetromino = true;
                        break;
                    }
                }
            }

            for (int i = 0; i < 4; ++i) {
                if (tetris->currentPositions[i].x == x && tetris->currentPositions[i].y == y) {
                    isCurrentTetromino = true;
                    break;
                }
            }

        // Only draw colors if toggleColors is true
            if (tetris->toggleColors) {
                if (isCurrentTetromino) {
                    printf("%s ", TETRIS_COLORS[tetris->currentTetromino]);
                } else if (isGhostTetromino) {
                    printf("%s ", TETRIS_COLORS[GHOST_COLOR_INDEX]);
                } else if (tetris->board[y][x] >= 0 && tetris->board[y][x] <= 7) {
                    printf("%s ", TETRIS_COLORS[tetris->board[y][x]]);
                } else if (tetris->board[y][x] >= -7 && tetris->board[y][x] < 0) {
                    printf("%s ", TETRIS_COLORS[-tetris->board[y][x]]);
                } else { // Cell is empty
                    // Modify this else block to control dot visibility
                    if(tetris->showDots) {
                        putchar('.'); // Show dot
                    } else {
                        putchar(' '); // Hide dot (print space)
                    }                    
                }
                printf("\033[0m");  // Reset color after drawing each cell
            } else {
                // When not using color, draw the pieces and control dot visibility
                if (isCurrentTetromino || (tetris->board[y][x] >= -7 && tetris->board[y][x] <= 7)) {
                    putchar('#');
                    } else if (isGhostTetromino) {
                    putchar('*');
                } else { // Cell is empty
                    // Modify this else block to control dot visibility
                    if(tetris->showDots) {
                        putchar('.'); // Show dot
                    } else {
                        putchar(' '); // Hide dot (print space)
                    }                    
                }
            }
        }

            
        printf("\u2502"); // Vertical line at the end of the board row
        
        if (y == 0) {
            printf("  Score: %d", tetris->score);
        } else if (y == 1) {
            printf("  Level: %d", tetris->level);
        } else if (y == 2) {
            printf("  Lines: %d", tetris->linesCleared);
        } else if (y == 4) {
            printf("  Next:");
        } else if (y >= 6 && y < 10) {
            printf("  ");
            drawNextTetromino(tetris->nextTetromino, y - 6, tetris);
        } else if (y == 12) {
            printf("  Controls:");
        } else if (y >= 13 && y <= 17) {
            const char *controls[] = {
                "A: Move left   C: Toggle color control",
                "D: Move right  T: Toggle dots visibility",
                "S: Soft drop",
                "W: Rotate",
                "Space: Hard drop"};
            printf("  %s", controls[y - 13]);
        } else if (y == 18) {
            printf("  G: Toggle ghost pieces");
        } else if (y == 19) {
            printf("  Q: Quit the game");
        } else if (y == 20) {
            printf("  P: Pause the game");
        }
        putchar('\n');
    }
    // Draw bottom border
    printf("\033[%d;%dH\u2570", vertical_padding + BOARD_HEIGHT + 1, horizontal_padding); // Position to the left bottom corner plus vertical padding
    for (int i = 0; i < BOARD_WIDTH; ++i) printf("\u2500");
    printf("\u256F\n");  // Bottom-right corner

    printf("\033[0m"); // Reset colors and styles

    // Reset color to default after drawing the border
#ifdef _WIN32
    SetConsoleTextAttribute(consoleHandle, 7); // Resets to default color
#else
    printf("\033[0m"); // Resets to default color on UNIX systems
#endif
}

void input(Tetris *tetris) {
    if (_kbhit()) {
        char key = _getch();
        if(tetris->paused && key != 'p') // If game is paused and key pressed is not 'p', return immediately
          return;
        switch (key) {
            case 'a':
                tetris_move(tetris, -1, 0);
                break;
            case 'd':
                tetris_move(tetris, 1, 0);
                break;
            case 's':
                tetris_move(tetris, 0, 1);
                break;
            case 'w':
                rotate(tetris);
                break;
            case ' ':
                while (tetris_move(tetris, 0, 1)) {}
                lockTetromino(tetris);
		removeFullLines(tetris);
		spawnTetromino(tetris); 
                break;
            case 'q':
                tetris->gameOver = true;
                break;
            case 'p':
                tetris->paused = !tetris->paused;
                return;
                break;
            case 'g':
                tetris->showGhost = !tetris->showGhost;
                break;
            case 'c':         
                tetris->toggleColors = !tetris->toggleColors;
                break;
            case 't':  
                tetris->showDots = !tetris->showDots;
                break;
        }
    }
}

void update(Tetris *tetris) {
    static uint64_t lastUpdateTimeMillis = 0;
    uint64_t currentTimeMillis;
    int speed = 1000 - (tetris->level - 1) * 100;

#ifdef _WIN32
    LARGE_INTEGER frequency, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&currentTime);
    currentTimeMillis = (currentTime.QuadPart * 1000) / frequency.QuadPart;
#else
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    currentTimeMillis = tspec.tv_sec * 1000 + tspec.tv_nsec / 1000000;
#endif

    if (currentTimeMillis - lastUpdateTimeMillis >= speed) {
        if (!tetris_move(tetris, 0, 1)) {
            lockTetromino(tetris);
            removeFullLines(tetris);
            spawnTetromino(tetris);

            if (!isValidPosition(tetris, tetris->currentPositions)) {
                tetris->gameOver = true;
            }
        }
        lastUpdateTimeMillis = currentTimeMillis;
    } else {
#ifdef _WIN32
static uint64_t lastFrameTime = 0;
uint64_t currentTime = getCurrentTimeMillis();
uint64_t elapsedTime = currentTime - lastFrameTime;
if (elapsedTime < 16) {
    Sleep(16 - elapsedTime);
}
lastFrameTime = currentTime;
#else
        usleep(16000);
#endif
    }
}

bool tetris_move(Tetris *tetris, int dx, int dy) {
    Point newPositions[4];
    for (int i = 0; i < 4; ++i) {
        newPositions[i] = (Point){tetris->currentPositions[i].x + dx, tetris->currentPositions[i].y + dy};
    }
    if (isValidPosition(tetris, newPositions)) {
        for (int i = 0; i < 4; ++i) {
            tetris->currentPositions[i] = newPositions[i];
        }
        return true;
    }
    return false;
}

void rotatePoint(const Point *pivot, Point *point) {
    int x = point->x - pivot->x;
    int y = point->y - pivot->y;
    point->x = pivot->x - y;
    point->y = pivot->y + x;
}

void rotate(Tetris *tetris) {
    Point newPositions[4];
    memcpy(newPositions, tetris->currentPositions, sizeof(Point) * 4);

    int pivotIndex;
    switch (tetris->currentTetromino) {
        case I:
        case J:
        case L:
        case T: 
            pivotIndex = 1; 
            break;
        case S:
        case Z:
            pivotIndex = 0;
            break;
        case O:
            return; 
        case INV_L: 
            pivotIndex = 2; 
            break;
    }

    for (int i = 0; i < 4; ++i) {
        if (i != pivotIndex) {
            rotatePoint(&newPositions[pivotIndex], &newPositions[i]);
        }
    }

    int offsetX[] = {0, 1, -1, 2, -2};
    for (int i = 0; i < 5; ++i) {
        bool valid = true;
        for (int j = 0; j < 4; ++j) {
            int x = newPositions[j].x + offsetX[i];
            int y = newPositions[j].y;
            if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT || tetris->board[y][x] != '.') {
                valid = false;
                break;
            }
        }
        if (valid) {
            for (int j = 0; j < 4; ++j) {
                tetris->currentPositions[j].x = newPositions[j].x + offsetX[i];
                tetris->currentPositions[j].y = newPositions[j].y;
            }
            break;
        }
    }
}

bool isValidPosition(const Tetris *tetris, const Point *positions) {
    for (int i = 0; i < 4; ++i) {
        int x = positions[i].x;
        int y = positions[i].y;
        if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) {
            return false;
        }
        if (tetris->board[y][x] != '.') {
            return false;
        }
    }
    return true;
}

void lockTetromino(Tetris *tetris) {
    for (int i = 0; i < 4; ++i) {
        int value = tetris->toggleColors ? tetris->currentTetromino : -tetris->currentTetromino;
        tetris->board[tetris->currentPositions[i].y][tetris->currentPositions[i].x] = value;
    }
}

int _kbhit() {
#ifdef _WIN32
    return kbhit();
#else
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
#endif
}

int _getch() {
#ifdef _WIN32
    return getch();
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}
