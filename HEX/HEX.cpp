#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <algorithm>
#include <windows.h>
#include <iomanip>

using namespace std;

class HexGame {
private:
    int N;
    vector<vector<char>> board;

public:
    HexGame(int n) : N(n), board(n, vector<char>(n, '.')) {}

    void printBoard() const {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        system("cls");

        SetConsoleTextAttribute(hConsole, 14);
        cout << "\n     *** ИГРА HEX ***    Размер: " << N << "x" << N << "\n\n";
        SetConsoleTextAttribute(hConsole, 15);

        cout << "    ";
        for (int c = 0; c < N; ++c) {
            SetConsoleTextAttribute(hConsole, 11);
            cout << setw(2) << c;
            SetConsoleTextAttribute(hConsole, 15);
        }
        cout << "\n\n";

        for (int r = 0; r < N; ++r) {
            SetConsoleTextAttribute(hConsole, 10);
            cout << string(r, ' ') << r << " ";
            SetConsoleTextAttribute(hConsole, 15);

            for (int c = 0; c < N; ++c) {
                char cell = board[r][c];
                if (cell == 'X') {
                    SetConsoleTextAttribute(hConsole, 12);
                    cout << "XX ";
                }
                else if (cell == 'O') {
                    SetConsoleTextAttribute(hConsole, 9);
                    cout << "OO ";
                }
                else {
                    SetConsoleTextAttribute(hConsole, 8);
                    cout << ".. ";
                }
            }
            SetConsoleTextAttribute(hConsole, 15);
            cout << "\n";
        }

        SetConsoleTextAttribute(hConsole, 12);
        cout << "\n X<---ЛЕВАЯ----ПРАВАЯ--->O\n";
        SetConsoleTextAttribute(hConsole, 10);
        cout << " |---ВЕРХНЯЯ--НИЖНЯЯ---|\n\n";
        SetConsoleTextAttribute(hConsole, 7);
    }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < N && c >= 0 && c < N;
    }

    bool makeMove(int r, int c, char player) {
        if (!inBounds(r, c) || board[r][c] != '.') return false;
        board[r][c] = player;
        return true;
    }

    void undoMove(int r, int c) {
        if (inBounds(r, c)) board[r][c] = '.';
    }

    bool isCellEmpty(int r, int c) const {
        if (!inBounds(r, c)) return false;
        return board[r][c] == '.';
    }

    char getCell(int r, int c) const {
        if (!inBounds(r, c)) return '#';
        return board[r][c];
    }

    bool checkWin(char player) const {
        vector<vector<bool>> visited(N, vector<bool>(N, false));
        queue<pair<int, int>> q;
        const int dr[6] = { -1, -1, 0, 0, 1, 1 };
        const int dc[6] = { 0, 1, -1, 1, -1, 0 };

        if (player == 'X') {
            for (int r = 0; r < N; ++r) {
                if (board[r][0] == player) {
                    q.push(make_pair(r, 0));
                    visited[r][0] = true;
                }
            }
            while (!q.empty()) {
                pair<int, int> cell = q.front(); q.pop();
                int r = cell.first;
                int c = cell.second;
                if (c == N - 1) return true;
                for (int k = 0; k < 6; ++k) {
                    int nr = r + dr[k], nc = c + dc[k];
                    if (inBounds(nr, nc) && !visited[nr][nc] && board[nr][nc] == player) {
                        visited[nr][nc] = true;
                        q.push(make_pair(nr, nc));
                    }
                }
            }
        }
        else {
            for (int c = 0; c < N; ++c) {
                if (board[0][c] == player) {
                    q.push(make_pair(0, c));
                    visited[0][c] = true;
                }
            }
            while (!q.empty()) {
                pair<int, int> cell = q.front(); q.pop();
                int r = cell.first;
                int c = cell.second;
                if (r == N - 1) return true;
                for (int k = 0; k < 6; ++k) {
                    int nr = r + dr[k], nc = c + dc[k];
                    if (inBounds(nr, nc) && !visited[nr][nc] && board[nr][nc] == player) {
                        visited[nr][nc] = true;
                        q.push(make_pair(nr, nc));
                    }
                }
            }
        }
        return false;
    }

    int getSize() const { return N; }

    bool isFull() const {
        for (auto& row : board)
            for (char c : row)
                if (c == '.')
                    return false;
        return true;
    }
};

class SmarterAI {
public:
    SmarterAI(char aiChar, int depth = 2)
        : playerChar(aiChar), opponentChar(aiChar == 'X' ? 'O' : 'X'), maxDepth(depth) {}

    pair<int, int> chooseMove(HexGame& game) {
        int bestScore = INT_MIN;
        pair<int, int> bestMove = make_pair(-1, -1);
        int N = game.getSize();

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                if (game.isCellEmpty(r, c)) {
                    game.makeMove(r, c, playerChar);
                    int score = minimax(game, maxDepth - 1, false, INT_MIN, INT_MAX);
                    game.undoMove(r, c);
                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = make_pair(r, c);
                    }
                }
            }
        }
        return bestMove;
    }

private:
    char playerChar;
    char opponentChar;
    int maxDepth;

    int minimax(HexGame& game, int depth, bool isMaximizing, int alpha, int beta) {
        if (game.checkWin(playerChar)) return 100000 - (maxDepth - depth);
        if (game.checkWin(opponentChar)) return -100000 + (maxDepth - depth);
        if (depth == 0 || game.isFull()) return evaluateBoard(game);

        int N = game.getSize();
        int bestScore = isMaximizing ? INT_MIN : INT_MAX;

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                if (game.isCellEmpty(r, c)) {
                    game.makeMove(r, c, isMaximizing ? playerChar : opponentChar);
                    int score = minimax(game, depth - 1, !isMaximizing, alpha, beta);
                    game.undoMove(r, c);

                    if (isMaximizing) {
                        bestScore = max(bestScore, score);
                        alpha = max(alpha, bestScore);
                    }
                    else {
                        bestScore = min(bestScore, score);
                        beta = min(beta, bestScore);
                    }
                    if (beta <= alpha)
                        return bestScore;
                }
            }
        }
        return bestScore;
    }

    int evaluateBoard(const HexGame& game) {
        return scorePlayer(game, playerChar) - scorePlayer(game, opponentChar);
    }

    int scorePlayer(const HexGame& game, char player) {
        int score = 0;
        int N = game.getSize();
        const int dr[6] = { -1, -1, 0, 0, 1, 1 };
        const int dc[6] = { 0, 1, -1, 1, -1, 0 };

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                if (game.getCell(r, c) == player) {
                    score += 5;
                    for (int k = 0; k < 6; ++k) {
                        int nr = r + dr[k], nc = c + dc[k];
                        if (game.inBounds(nr, nc) && game.getCell(nr, nc) == player)
                            score += 3;
                    }
                }
            }
        }
        return score;
    }
};

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    setlocale(LC_ALL, "Russian");

    int N;
    SetConsoleTextAttribute(hConsole, 14);
    cout << "*** Добро пожаловать в ИГРУ HEX! ***\n\n";
    SetConsoleTextAttribute(hConsole, 15);
    cout << "Размер поля (5-12): ";
    cin >> N;
    if (N < 2 || N > 20) N = 7;

    HexGame game(N);

    cout << "\nРежим:\n1 - 2 игрока\n2 - vs УМНЫЙ ИИ\nВыбор: ";
    int mode;
    cin >> mode;

    if (mode == 1) {
        char current = 'X';
        while (true) {
            game.printBoard();
            SetConsoleTextAttribute(hConsole, current == 'X' ? 12 : 9);
            cout << "\nХод " << current << " (строка столбец): ";
            SetConsoleTextAttribute(hConsole, 15);
            int r, c;
            cin >> r >> c;

            if (!game.makeMove(r, c, current)) {
                SetConsoleTextAttribute(hConsole, 12);
                cout << "Неверный ход!\n";
                SetConsoleTextAttribute(hConsole, 15);
                system("pause");
                continue;
            }

            if (game.checkWin(current)) {
                game.printBoard();
                SetConsoleTextAttribute(hConsole, 14);
                cout << "\nПОБЕДИЛ " << current << "!!!\n";
                break;
            }

            if (game.isFull()) {
                game.printBoard();
                SetConsoleTextAttribute(hConsole, 14);
                cout << "\nНИЧЬЯ!\n";
                break;
            }

            current = (current == 'X' ? 'O' : 'X');
        }
    }
    else {
        SmarterAI ai('O', 2);
        char human = 'X';
        char current = 'X';
        while (true) {
            game.printBoard();
            if (current == human) {
                SetConsoleTextAttribute(hConsole, 10);
                cout << "\nВаш ход: ";
                SetConsoleTextAttribute(hConsole, 15);
                int r, c;
                cin >> r >> c;
                if (!game.makeMove(r, c, human)) {
                    SetConsoleTextAttribute(hConsole, 12);
                    cout << "Неверный ход!\n";
                    SetConsoleTextAttribute(hConsole, 15);
                    system("pause");
                    continue;
                }
            }
            else {
                SetConsoleTextAttribute(hConsole, 12);
                cout << "\nИИ думает";
                for (int i = 0; i < 3; ++i) {
                    cout << ".";
                    Sleep(300);
                }
                cout << "\n";
                SetConsoleTextAttribute(hConsole, 15);

                pair<int, int> move = ai.chooseMove(game);
                if (move.first == -1) {
                    SetConsoleTextAttribute(hConsole, 14);
                    cout << "НИЧЬЯ!\n";
                    break;
                }
                game.makeMove(move.first, move.second, 'O');
                SetConsoleTextAttribute(hConsole, 12);
                cout << "ИИ: (" << move.first << "," << move.second << ")\n";
                SetConsoleTextAttribute(hConsole, 15);
            }

            if (game.checkWin(human)) {
                game.printBoard();
                SetConsoleTextAttribute(hConsole, 10);
                cout << "\nВЫ ПОБЕДИЛИ!\n";
                break;
            }
            if (game.checkWin('O')) {
                game.printBoard();
                SetConsoleTextAttribute(hConsole, 12);
                cout << "\nИИ ПОБЕДИЛ!\n";
                break;
            }
            if (game.isFull()) {
                game.printBoard();
                SetConsoleTextAttribute(hConsole, 14);
                cout << "\nНИЧЬЯ!\n";
                break;
            }

            current = (current == 'X' ? 'O' : 'X');
        }
    }

    SetConsoleTextAttribute(hConsole, 14);
    cout << "\nНажмите любую клавишу... ";
    SetConsoleTextAttribute(hConsole, 15);
    system("pause");
    return 0;
}