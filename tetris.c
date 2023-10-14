#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h> // Add this line
#include <sys/ioctl.h>
#endif

#define BOARD_WIDTH 20
#define BOARD_HEIGHT 21

typedef enum {
    I, J, L, O, S, T, Z, INV_L
} Tetromino;

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
} Tetris;

void spawnTetromino(Tetris *tetris);
void draw(const Tetris *tetris);
void drawGhost(const Tetris *tetris);
void drawNextTetromino(Tetromino tetromino, int row);
void input(Tetris *tetris);
void update(Tetris *tetris);
bool move(Tetris *tetris, int dx, int dy);
void rotate(Tetris *tetris);
bool isValidPosition(const Tetris *tetris, const Point *positions);
void lockTetromino(Tetris *tetris);
void removeFullLines(Tetris *tetris);
void drawGameOverScreen(const Tetris *tetris, int score, int level, int linesCleared);
int _kbhit();
int _getch();

int main() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    Tetris tetris = {0};
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            tetris.board[y][x] = '.';
        }
    }
    srand(time(0));
    tetris.nextTetromino = (rand() % 8); // Initialize the next Tetromino       
    spawnTetromino(&tetris);
    tetris.level = 1;
    tetris.linesCleared = 0;
    tetris.paused = false;
    tetris.showGhost = true;

    bool game_over_screen_displayed = false;

    // Game loop
while (1) {
    if (!tetris.paused && !tetris.gameOver) {
        draw(&tetris);
        input(&tetris);
        update(&tetris);
    } else if (tetris.gameOver && !game_over_screen_displayed) {
        game_over_screen_displayed = true;
        drawGameOverScreen(&tetris, tetris.score, tetris.level, tetris.linesCleared);
        // Wait for user input to exit the game
        _getch();
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
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
        Sleep(50);
    #else
        usleep(50 * 1000);
    #endif
        input(&tetris);
        }
    }

    return 0;
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

    // Update the player's score and lines cleared based on the number of lines removed
    if (linesRemoved > 0) {
        int lineScore[] = {0, 100, 300, 500, 800};
        int bonus = (linesRemoved - 1) * 100; // Add a bonus for clearing multiple lines at once
        tetris->score += (lineScore[linesRemoved] + bonus) * tetris->level;
        tetris->linesCleared += linesRemoved;

        if (tetris->linesCleared >= tetris->level * 10) {
            tetris->level++;
        }
    }
}

void spawnTetromino(Tetris *tetris) {
    // Set the current Tetromino and generate the next one
    tetris->currentTetromino = tetris->nextTetromino;
    tetris->nextTetromino = (rand() % 8);
    tetris->rotation = 0;

    const char *shape = TETROMINO_SHAPES[tetris->currentTetromino][tetris->rotation];
    int idx = 0;
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            if (shape[y * 4 + x] == '#') {
                tetris->currentPositions[idx++] = (Point){x + BOARD_WIDTH / 2 - 2, y};
            }
        }
    }
}

void drawNextTetromino(Tetromino tetromino, int row) {
    const char *shape = TETROMINO_SHAPES[tetromino][0];
    for (int x = 0; x < 4; ++x) {
        if (shape[row * 4 + x] == '#') {
            putchar('#');
        } else {
            putchar(' ');
        }
    }
}

void drawGameOverScreen(const Tetris *tetris, int score, int level, int linesCleared) {
    // Call draw function to draw the game board
    draw(tetris);

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int horizontal_padding = (csbi.srWindow.Right - csbi.srWindow.Left + 1 - BOARD_WIDTH) / 2;
    int vertical_padding = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1 - 9) / 2;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int horizontal_padding = (w.ws_col - BOARD_WIDTH) / 2;
    int vertical_padding = (w.ws_row - 9) / 2;
#endif

    // Position the cursor at the starting point of the game over screen
    printf("\033[%d;%dH", vertical_padding);

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
        printf("\033[E"); // Move the cursor to the next line
    }
}

void draw(const Tetris *tetris) {

printf("\033[1;1H");

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int horizontal_padding = (csbi.srWindow.Right - csbi.srWindow.Left + 1 - BOARD_WIDTH) / 2;
    int vertical_padding = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1 - BOARD_HEIGHT) / 2;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int horizontal_padding = (w.ws_col - BOARD_WIDTH) / 2;
    int vertical_padding = (w.ws_row - BOARD_HEIGHT) / 2;
#endif

    for (int i = 0; i < vertical_padding; ++i) {
        putchar('\n');
    }

    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int i = 0; i < horizontal_padding; ++i) {
            putchar(' ');
        }
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

            if (isCurrentTetromino) {
                putchar('#');
            } else if (isGhostTetromino) {
                putchar('*');
            } else {
                putchar(tetris->board[y][x]);
            }
        }
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
            drawNextTetromino(tetris->nextTetromino, y - 6);
        } else if (y == 12) {
            printf("  Controls:");
        } else if (y >= 13 && y <= 17) {
            const char *controls[] = {
                "A: Move left",
                "D: Move right",
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
}

void input(Tetris *tetris) {
    if (_kbhit()) {
        char key = _getch();
        switch (key) {
            case 'a':
                move(tetris, -1, 0);
                break;
            case 'd':
                move(tetris, 1, 0);
                break;
            case 's':
                move(tetris, 0, 1);
                break;
            case 'w':
                rotate(tetris);
                break;
            case ' ':
                while (move(tetris, 0, 1)) {}
                lockTetromino(tetris);
                break;
            case 'q':
                tetris->gameOver = true;
                break;
            case 'p':
                tetris->paused = !tetris->paused;
                break;
            case 'g':
                tetris->showGhost = !tetris->showGhost;
                break;
        }
    }
}

void update(Tetris *tetris) {
    static int timeCounter = 0;
#ifdef _WIN32
    int speed = 110 - (tetris->level * 10); // Adjust speed based on level
#else
    int speed = 1000 - (tetris->level - 1) * 100; // Adjust speed based on level
#endif

    if (timeCounter >= speed) {
        if (!move(tetris, 0, 1)) {
            lockTetromino(tetris);
            spawnTetromino(tetris);

            // Check for game over condition (new Tetromino spawned in an invalid position)
            if (!isValidPosition(tetris, tetris->currentPositions)) {
                tetris->gameOver = true;
            }
        }
        timeCounter = 0;
    } else {
        timeCounter += 25;
    }

#ifdef _WIN32
    Sleep(25);
#else
    usleep(25 * 1000);
#endif
}


bool move(Tetris *tetris, int dx, int dy) {
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
            pivotIndex = 2;
            break;
        case J:
        case L:
        case S:
        case T:
        case Z:
            pivotIndex = 1;
            break;
        case O:
            return; // O shape doesn't need rotation
    }

    for (int i = 0; i < 4; ++i) {
        if (i != pivotIndex) {
            rotatePoint(&newPositions[pivotIndex], &newPositions[i]);
        }
    }

    // Wall kick
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
        int x = tetris->currentPositions[i].x;
        int y = tetris->currentPositions[i].y;
        tetris->board[y][x] = '#';
    }
    removeFullLines(tetris);
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

    if (ch != (char)EOF) {
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