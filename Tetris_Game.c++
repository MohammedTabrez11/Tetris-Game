#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <thread>
#include <stdio.h>

// Cross-platform includes
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <locale.h>
#endif

using namespace std;

// Platform independent sleep function
void sleepFor(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Cross-platform console functions
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void gotoxy(int x, int y) {
    #ifdef _WIN32
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    #else
        printf("\033[%d;%dH", y + 1, x + 1);
    #endif
}

void hideCursor() {
    #ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = FALSE;
        SetConsoleCursorInfo(consoleHandle, &info);
    #else
        cout << "\033[?25l";
    #endif
}

// Cross-platform keyboard input
int kbhit() {
    #ifdef _WIN32
        return _kbhit();
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
        if(ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    #endif
}

int getch() {
    #ifdef _WIN32
        return _getch();
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

class Tetris {
private:
    static const int WIDTH = 10, HEIGHT = 20;
    static const int LEVEL_UP_SCORE = 500;
    static const int BASE_FALL_SPEED = 500;

    int grid[HEIGHT][WIDTH];
    int score;
    int highScore;
    int level;
    int tetX;
    int tetY;
    int currentTetrominoIndex;
    int nextTetrominoIndex;
    
    vector<vector<int>> currentTetromino;
    vector<vector<int>> nextTetromino;
    
    static const vector<string> color;
    static const vector<vector<vector<int>>> tetrominoes;

    bool isValidPosition(const vector<vector<int>>& shape, int x, int y) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shape[i][j]) {
                    int newX = x + j;
                    int newY = y + i;
                    if (newX < 0 || newX >= WIDTH || newY >= HEIGHT)
                        return false;
                    if (newY >= 0 && grid[newY][newX] >= 0)
                        return false;
                }
            }
        }
        return true;
    }

    vector<vector<int>> rotateTetromino(const vector<vector<int>>& shape) {
        vector<vector<int>> rotated(4, vector<int>(4, 0));
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                rotated[j][3 - i] = shape[i][j];
            }
        }
        return rotated;
    }

    void loadHighScore() {
        ifstream file("highscore.txt");
        if (file >> highScore)
            file.close();
    }

    void saveHighScore() {
        if (score > highScore) {
            ofstream file("highscore.txt");
            file << score;
            file.close();
        }
    }

    int getFallSpeed() {
        return max(100, BASE_FALL_SPEED - (level * 50));
    }

    void displayBonusText(int bonus, int x, int y) {
        gotoxy(x, y);
        
        for (int i = 0; i < 3; i++) {
            gotoxy(x, y);
            cout << "🎉 BONUS +" << bonus << " 🎉";
            sleepFor(200);
            gotoxy(x, y);
            cout << "   BONUS +" << bonus << "   ";
            sleepFor(200);
        }

        gotoxy(x, y);
        cout << "                    ";
    }

    void getNextTetromino() {
        nextTetrominoIndex = rand() % tetrominoes.size();
        nextTetromino = tetrominoes[nextTetrominoIndex];
    }

    void displayNextTetromino() {
        int x = WIDTH * 2 + 5;
        int y = 2;
        gotoxy(x, y);
        
        cout << "Next Piece:";
        y++;
        gotoxy(x, y);
        
        for(int i = 0; i < 4; i++) {
            cout << "  ";
            for(int j = 0; j < 4; j++) {
                cout << (nextTetromino[i][j] ? color[nextTetrominoIndex] : "⬜");
            }
            cout << "\n";
            y++;
            gotoxy(x, y);
        }
    }

    void displayGrid() {
        gotoxy(0, 0);
        cout << "\n";

        int tempGrid[HEIGHT][WIDTH];
        memcpy(tempGrid, grid, sizeof(grid));

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (currentTetromino[i][j] && tetY + i < HEIGHT && tetX + j < WIDTH && tetX + j >= 0) {
                    tempGrid[tetY + i][tetX + j] = currentTetrominoIndex;
                }
            }
        }

        for (int i = 0; i < HEIGHT; i++) {
            cout << "|";
            for (int j = 0; j < WIDTH; j++) {
                cout << (tempGrid[i][j] >= 0 ? color[tempGrid[i][j]] : "⬜");
            }
            cout << "|";
            
            if (i < 6) {
                cout << "  ";
                if (i == 0) cout << "Next Piece:";
                else if (i > 1) {
                    for(int j = 0; j < 4; j++) {
                        cout << (nextTetromino[i-2][j] ? color[nextTetrominoIndex] : "⬜");
                    }
                }
            }
            else if (i == 7) {
                cout << "  ╔════════════════════╗";
            }
            else if (i == 8) {
                cout << "  ║ Score: " << setw(11) << score << " ║";
            }
            else if (i == 9) {
                cout << "  ╚════════════════════╝";
            }
            else if (i == 10) {
                cout << "  ╔════════════════════╗"; 
            }
            else if (i == 11) {
                cout << "  ║ High: " << setw(12) << highScore << " ║";
            }
            else if(i == 12){
                cout << "  ╚════════════════════╝";
            }
            else if(i == 13){
                cout << "  ╔════════════════════╗";
            }
            else if(i == 14){
                cout << "  ║ Level: " << setw(11) << level << " ║";
            }
            else if(i == 15){
                cout << "  ╚════════════════════╝";
            }
            else if(i == 16){
                cout << "  ╔══════════════════════════════════════════════════╗";
            }
            else if(i == 17){
                cout << "  ║ Controls: WASD Move, W Rotate, R Restart, Q Quit ║";
            }
            else if(i == 18){
                cout << "  ╚══════════════════════════════════════════════════╝";
            }
            cout << "\n";
        }
    }

    void mergeTetromino() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (currentTetromino[i][j] && tetY + i >= 0) {
                    grid[tetY + i][tetX + j] = currentTetrominoIndex;
                }
            }
        }
    }

    void animateLineClear(int y) {
        gotoxy(0, y + 1);
        
        for(int i = 0; i < 3; i++) {
            gotoxy(0, y + 1);
            cout << "|";
            for(int j = 0; j < WIDTH; j++) {
                cout << "✨";
            }
            cout << "|";
            sleepFor(100);
            gotoxy(0, y + 1);
            cout << "|";
            for(int j = 0; j < WIDTH; j++) {
                cout << "⬜";
            }
            cout << "|";
            sleepFor(100);
        }
    }

    void clearLines() {
        int linesCleared = 0;
        for (int y = HEIGHT - 1; y >= 0; y--) {
            bool complete = true;
            for (int x = 0; x < WIDTH; x++) {
                if (grid[y][x] < 0) {
                    complete = false;
                    break;
                }
            }

            if (complete) {
                animateLineClear(y);
                
                for (int ny = y; ny > 0; ny--) {
                    for (int x = 0; x < WIDTH; x++) {
                        grid[ny][x] = grid[ny - 1][x];
                    }
                }
                
                for (int x = 0; x < WIDTH; x++) {
                    grid[0][x] = -1;
                }
                
                score += 100;
                if (score % LEVEL_UP_SCORE == 0) {
                    level++;
                }
                linesCleared++;
                y++;

                displayGrid();
            }
        }

        if (linesCleared > 1) {
            int bonus = (linesCleared - 1) * 50;
            score += bonus;
            if (score % LEVEL_UP_SCORE == 0) {
                level++;
            }
            displayBonusText(bonus, WIDTH / 2 - 5, 0);
        }
    }

    void displayRules() {
        system("cls");
        cout << "╔════════════════════════════════════════════════════════════════════════════════╗\n";
        cout << "║                              TETRIS GAME RULES                                 ║\n";
        cout << "╠════════════════════════════════════════════════════════════════════════════════╣\n";
        cout << "║                                                                                ║\n";
        cout << "║  CONTROLS:                                                                     ║\n";
        cout << "║    A/← - Move piece left                                                       ║\n";
        cout << "║    D/→ - Move piece right                                                      ║\n";
        cout << "║    S/↓ - Soft drop (move piece down faster)                                    ║\n";
        cout << "║    W/↑ - Rotate piece clockwise                                                ║\n";
        cout << "║    Space - Hard drop (instantly drop piece)                                    ║\n";
        cout << "║    R - Restart game                                                            ║\n";
        cout << "║    Q - Quit game                                                               ║\n";
        cout << "║    P - Pause game                                                              ║\n";
        cout << "║                                                                                ║\n";
        cout << "║  SCORING:                                                                      ║\n";
        cout << "║    • 100 points for each line cleared                                          ║\n";
        cout << "║    • Bonus points for multiple lines:                                          ║\n";
        cout << "║      - 2 lines: +50 bonus                                                      ║\n";
        cout << "║      - 3 lines: +100 bonus                                                     ║\n";
        cout << "║      - 4 lines: +150 bonus                                                     ║\n";
        cout << "║                                                                                ║\n";
        cout << "║  LEVELS:                                                                       ║\n";
        cout << "║    • Start at Level 1                                                          ║\n";
        cout << "║    • Level up every 500 points                                                 ║\n";
        cout << "║    • Pieces fall faster each level                                             ║\n";
        cout << "║                                                                                ║\n";
        cout << "║  TIPS:                                                                         ║\n";
        cout << "║    • Keep the playing field flat                                               ║\n";
        cout << "║    • Create spaces for I pieces                                                ║\n";
        cout << "║    • Use hard drop for quick placement                                         ║\n";
        cout << "║    • Plan ahead for rotations                                                  ║\n";
        cout << "║    • Clear lines from bottom up                                                ║\n";
        cout << "║                                                                                ║\n";
        cout << "║  Press any key to start the game...                                            ║\n";
        cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n";
        _getch();
    }

public:
    Tetris() : score(0), level(1), tetX(WIDTH / 2 - 2), tetY(0), currentTetrominoIndex(0), nextTetrominoIndex(0) {
        memset(grid, -1, sizeof(grid));
        loadHighScore();
    }

    void run() {
        displayRules();
        clearScreen();
        hideCursor();
        srand(time(0));

        getNextTetromino();

        while (true) {
            currentTetromino = nextTetromino;
            currentTetrominoIndex = nextTetrominoIndex;
            
            getNextTetromino();
            
            tetX = WIDTH / 2 - 2;
            tetY = 0;
            if (!isValidPosition(currentTetromino, tetX, tetY)) {
                cout << "Press 'p' to play again or 'q' to quit\n";
                char ch;
                while (true) {
                    ch = getch();
                    if (ch == 'P' || ch == 'p') {
                        run();
                    }
                    else if (ch == 'q' || ch == 'Q') {
                        saveHighScore();
                        return;
                    }
                }
            }

            while (true) {
                displayGrid();

                if (kbhit()) {
                    char key = getch();
                    key = tolower(key);
                    
                    if (key == 'a' && isValidPosition(currentTetromino, tetX - 1, tetY))
                        tetX--;
                    else if (key == 'd' && isValidPosition(currentTetromino, tetX + 1, tetY))
                        tetX++;
                    else if (key == 's') {
                        if (isValidPosition(currentTetromino, tetX, tetY + 1))
                            tetY++;
                        else {
                            mergeTetromino();
                            clearLines();
                            saveHighScore();
                            break;
                        }
                    }
                    else if (key == 'w') {
                        vector<vector<int>> rotated = rotateTetromino(currentTetromino);
                        if (isValidPosition(rotated, tetX, tetY)) {
                            currentTetromino = rotated;
                        }
                    }
                    else if (key == ' ') {
                        while (isValidPosition(currentTetromino, tetX, tetY + 1))
                            tetY++;
                        mergeTetromino();
                        clearLines();
                        saveHighScore();
                        break;
                    }
                    else if (key == 'r') {
                        run();
                        return;
                    }
                    else if (key == 'q') {
                        saveHighScore();
                        exit(0);
                    }
                    else if (key == 'p') {
                        while (true) {
                            char ch = getch();
                            sleepFor(100);
                            if (ch == 'p') {
                                break;
                            }
                        }
                    }
                }

                static auto lastFall = chrono::steady_clock::now();
                auto now = chrono::steady_clock::now();
                if (chrono::duration_cast<chrono::milliseconds>(now - lastFall).count() > getFallSpeed()) {
                    if (isValidPosition(currentTetromino, tetX, tetY + 1)) {
                        tetY++;
                    }
                    else {
                        mergeTetromino();
                        clearLines();
                        saveHighScore();
                        break;
                    }
                    lastFall = now;
                }

                sleepFor(10);
            }
        }
    }
};

// Static member initialization
const vector<string> Tetris::color = {
    "🟦", "🟪", "🟨", "🟥", "🟩", "🟧", "🟫"
};

const vector<vector<vector<int>>> Tetris::tetrominoes = {
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // I
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // T
    {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}, // O
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // Z
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // S
    {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // L
    {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}  // J
};

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #else
        setlocale(LC_ALL, "");
    #endif
    srand(time(0));
    Tetris game;
    game.run();
    return 0;
}
