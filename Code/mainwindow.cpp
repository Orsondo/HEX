#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QTimer>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QInputDialog>
#include <queue>
#include <vector>
#include <limits>

using std::vector;
using std::pair;
using std::queue;

static const int kHexDirections[6][2] = {
    {-1, 0}, {-1, 1}, {0, -1},
    {0, 1}, {1, -1}, {1, 0}
};

class HexGame {
public:
    explicit HexGame(int size) : size(size), board(size, vector<char>(size, '.')) {}

    bool makeMove(int r, int c, char player) {
        if (r < 0 || r >= size || c < 0 || c >= size || board[r][c] != '.') return false;
        board[r][c] = player;
        return true;
    }

    void undoMove(int r, int c) {
        if (r >= 0 && r < size && c >= 0 && c < size) board[r][c] = '.';
    }

    bool isCellEmpty(int r, int c) const {
        return r >= 0 && r < size && c >= 0 && c < size && board[r][c] == '.';
    }

    bool checkWin(char player) const {
        if (player == 'X') return hasHorizontalConnection();
        if (player == 'O') return hasVerticalConnection();
        return false;
    }

    bool isFull() const {
        for (const auto& row : board)
            for (char cell : row)
                if (cell == '.') return false;
        return true;
    }

    int getSize() const { return size; }
    const vector<vector<char>>& getBoard() const { return board; }

private:
    int size;
    vector<vector<char>> board;

    bool hasHorizontalConnection() const {
        vector<vector<bool>> visited(size, vector<bool>(size, false));
        queue<pair<int,int>> q;
        for (int r = 0; r < size; ++r) {
            if (board[r][0] == 'X') {
                q.push({r, 0});
                visited[r][0] = true;
            }
        }
        while (!q.empty()) {
            auto cur = q.front(); q.pop();
            int r = cur.first, c = cur.second;
            if (c == size - 1) return true;
            for (int d = 0; d < 6; ++d) {
                int nr = r + kHexDirections[d][0];
                int nc = c + kHexDirections[d][1];
                if (nr >= 0 && nr < size && nc >= 0 && nc < size &&
                    !visited[nr][nc] && board[nr][nc] == 'X') {
                    visited[nr][nc] = true;
                    q.push({nr, nc});
                }
            }
        }
        return false;
    }

    bool hasVerticalConnection() const {
        vector<vector<bool>> visited(size, vector<bool>(size, false));
        queue<pair<int,int>> q;
        for (int c = 0; c < size; ++c) {
            if (board[0][c] == 'O') {
                q.push({0, c});
                visited[0][c] = true;
            }
        }
        while (!q.empty()) {
            auto cur = q.front(); q.pop();
            int r = cur.first, c = cur.second;
            if (r == size - 1) return true;
            for (int d = 0; d < 6; ++d) {
                int nr = r + kHexDirections[d][0];
                int nc = c + kHexDirections[d][1];
                if (nr >= 0 && nr < size && nc >= 0 && nc < size &&
                    !visited[nr][nc] && board[nr][nc] == 'O') {
                    visited[nr][nc] = true;
                    q.push({nr, nc});
                }
            }
        }
        return false;
    }
};

int MainWindow::minMovesForXToWin(QVector<QPair<int,int>>* path) {
    const auto& board = game->getBoard();
    int size = boardSize;
    const int n = size * size;
    const int INF = 1'000'000'000;

    auto id = [size](int r, int c) { return r * size + c; };
    vector<int> dist(n, INF);
    vector<int> parent(n, -1);
    using Node = std::pair<int,int>;
    std::priority_queue<Node, vector<Node>, std::greater<Node>> pq;

    auto cellCost = [](char cell) -> int {
        if (cell == 'O') return std::numeric_limits<int>::max() / 4;
        return cell == 'X' ? 0 : 1;
    };

    for (int r = 0; r < size; ++r) {
        int cost = cellCost(board[r][0]);
        if (cost >= INF) continue;
        int idx = id(r, 0);
        dist[idx] = cost;
        pq.push({dist[idx], idx});
    }

    while (!pq.empty()) {
        auto [d, v] = pq.top();
        pq.pop();
        if (d != dist[v] || d >= INF) continue;
        int r = v / size;
        int c = v % size;
        for (int dir = 0; dir < 6; ++dir) {
            int nr = r + kHexDirections[dir][0];
            int nc = c + kHexDirections[dir][1];
            if (nr < 0 || nr >= size || nc < 0 || nc >= size) continue;
            char cell = board[nr][nc];
            if (cell == 'O') continue;
            int add = (cell == 'X') ? 0 : 1;
            int nv = id(nr, nc);
            if (dist[nv] > d + add) {
                dist[nv] = d + add;
                parent[nv] = v;
                pq.push({dist[nv], nv});
            }
        }
    }

    int bestIdx = -1;
    int bestCost = INF;
    for (int r = 0; r < size; ++r) {
        int idx = id(r, size - 1);
        if (dist[idx] < bestCost) {
            bestCost = dist[idx];
            bestIdx = idx;
        }
    }

    if (path && bestIdx != -1 && bestCost < INF) {
        QVector<QPair<int,int>> backtrack;
        int cur = bestIdx;
        while (cur != -1) {
            int r = cur / size;
            int c = cur % size;
            if (board[r][c] == '.') backtrack.prepend({r, c});
            cur = parent[cur];
        }
        *path = backtrack;
    }

    return bestCost >= INF ? INF : bestCost;
}

int MainWindow::minMovesForOToWin(QVector<QPair<int,int>>* path) {
    const auto& board = game->getBoard();
    int size = boardSize;
    const int n = size * size;
    const int INF = 1'000'000'000;

    auto id = [size](int r, int c) { return r * size + c; };
    vector<int> dist(n, INF);
    vector<int> parent(n, -1);
    using Node = std::pair<int,int>;
    std::priority_queue<Node, vector<Node>, std::greater<Node>> pq;

    auto cellCost = [](char cell) -> int {
        if (cell == 'X') return std::numeric_limits<int>::max() / 4;
        return cell == 'O' ? 0 : 1;
    };

    for (int c = 0; c < size; ++c) {
        int cost = cellCost(board[0][c]);
        if (cost >= INF) continue;
        int idx = id(0, c);
        dist[idx] = cost;
        pq.push({dist[idx], idx});
    }

    while (!pq.empty()) {
        auto [d, v] = pq.top();
        pq.pop();
        if (d != dist[v] || d >= INF) continue;
        int r = v / size;
        int c = v % size;
        for (int dir = 0; dir < 6; ++dir) {
            int nr = r + kHexDirections[dir][0];
            int nc = c + kHexDirections[dir][1];
            if (nr < 0 || nr >= size || nc < 0 || nc >= size) continue;
            char cell = board[nr][nc];
            if (cell == 'X') continue;
            int add = (cell == 'O') ? 0 : 1;
            int nv = id(nr, nc);
            if (dist[nv] > d + add) {
                dist[nv] = d + add;
                parent[nv] = v;
                pq.push({dist[nv], nv});
            }
        }
    }

    int bestIdx = -1;
    int bestCost = INF;
    for (int c = 0; c < size; ++c) {
        int idx = id(size - 1, c);
        if (dist[idx] < bestCost) {
            bestCost = dist[idx];
            bestIdx = idx;
        }
    }

    if (path && bestIdx != -1 && bestCost < INF) {
        QVector<QPair<int,int>> backtrack;
        int cur = bestIdx;
        while (cur != -1) {
            int r = cur / size;
            int c = cur % size;
            if (board[r][c] == '.') backtrack.prepend({r, c});
            cur = parent[cur];
        }
        *path = backtrack;
    }

    return bestCost >= INF ? INF : bestCost;
}

bool MainWindow::isXOneMoveFromWin() {
    int minMoves = minMovesForXToWin(nullptr);
    return minMoves <= 1;
}

bool MainWindow::isXTwoMovesFromWin() {
    int minMoves = minMovesForXToWin(nullptr);
    return minMoves <= 2;
}

int MainWindow::shortestPathToConnectO(int r, int c) {
    const auto& board = game->getBoard();
    int size = boardSize;
    int distToTop = r;
    int distToBottom = size - 1 - r;
    int xNearby = 0;
    for (int d = 0; d < 6; ++d) {
        int nr = r + kHexDirections[d][0];
        int nc = c + kHexDirections[d][1];
        if (nr >= 0 && nr < size && nc >= 0 && nc < size && board[nr][nc] == 'X') {
            xNearby++;
        }
    }
    return distToTop + distToBottom + xNearby * 2;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , game(nullptr)
    , boardSize(7)
    , currentPlayer('X')
    , vsAI(true)
    , statusLabel(nullptr)
    , gameGrid(nullptr)
    , aiFirstMove(true)
    , turnTimer(nullptr)
    , remainingSeconds(5)
    , lastXRow(-1)
    , lastXCol(-1)
    , gameOver(false)
{
    ui->setupUi(this);
    setupUI();
    showIntro();
    game = new HexGame(boardSize);
    newGame();
}

MainWindow::~MainWindow() {
    delete game;
    delete ui;
}

void MainWindow::setupUI() {
    setWindowTitle("HEX Qt – Быстрый и умный ИИ");
    resize(800, 800);
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    QLabel* title = new QLabel("HEX – соедините сторону\nX: слева‑направо, O: сверху‑вниз");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #FFD700;");
    mainLayout->addWidget(title);
    statusLabel = new QLabel("Ход X");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 16px; color: #FF4444; padding: 6px;");
    mainLayout->addWidget(statusLabel);
    gameGrid = new QGridLayout();
    gameGrid->setSpacing(2);
    gameGrid->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(gameGrid, 1);
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* newGameBtn = new QPushButton("Новая игра");
    QPushButton* menuBtn = new QPushButton("Правила");
    buttonsLayout->addWidget(newGameBtn);
    buttonsLayout->addWidget(menuBtn);
    mainLayout->addLayout(buttonsLayout);
    connect(newGameBtn, &QPushButton::clicked, this, &MainWindow::newGame);
    connect(menuBtn, &QPushButton::clicked, this, &MainWindow::showMenu);
}

void MainWindow::showIntro() {
    QMessageBox box(this);
    box.setWindowTitle("HEX");
    box.setText("Игра разработана \"Orsondo-3ITb-1\"");
    box.setStandardButtons(QMessageBox::NoButton);
    QTimer::singleShot(2000, &box, &QMessageBox::accept);
    box.exec();
}

void MainWindow::newGame() {
    bool ok = false;
    int size = QInputDialog::getInt(this, "Размер поля", "Введите размер поля (7–11):",
                                    boardSize, 7, 11, 1, &ok);
    if (!ok) return;

    QMessageBox modeBox(this);
    modeBox.setWindowTitle("Режим игры");
    modeBox.setText("Выберите режим:");
    QPushButton* aiBtn = modeBox.addButton("Против ИИ", QMessageBox::AcceptRole);
    QPushButton* localBtn = modeBox.addButton("На двоих локально", QMessageBox::DestructiveRole);
    modeBox.exec();
    vsAI = (modeBox.clickedButton() == aiBtn);

    boardSize = size;
    aiFirstMove = true;
    delete game;
    game = new HexGame(boardSize);
    currentPlayer = 'X';
    gameOver = false;
    lastXRow = -1;
    lastXCol = -1;
    buttons.assign(boardSize, vector<QPushButton*>(boardSize, nullptr));
    QLayoutItem* item;
    while ((item = gameGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    for (int r = 0; r < boardSize; ++r) {
        for (int c = 0; c < boardSize; ++c) {
            QPushButton* btn = new QPushButton("");
            btn->setFixedSize(48, 48);
            btn->setStyleSheet(
                "QPushButton { "
                "border: 2px solid #777; "
                "border-radius: 24px; "
                "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #DDDDDD, stop:1 #AAAAAA); "
                "font-size: 22px; font-weight: bold; "
                "}"
                "QPushButton:hover { "
                "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #EEEEEE, stop:1 #CCCCCC); "
                "}");
            buttons[r][c] = btn;
            int visualCol = c + r;
            gameGrid->addWidget(btn, r, visualCol);
            connect(btn, &QPushButton::clicked, this, [this, r, c]() {
                onCellClicked(r, c);
            });
        }
    }
    updateBoard();
    printStatus("Твой ход X");
    startTurnTimer("Твой ход X");
}

void MainWindow::startTurnTimer(const QString& baseStatus) {
    if (!turnTimer) {
        turnTimer = new QTimer(this);
        connect(turnTimer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    }
    if (turnTimer->isActive()) turnTimer->stop();
    remainingSeconds = 5;
    printStatus(QString("%1 | %2 c").arg(baseStatus).arg(remainingSeconds));
    turnTimer->start(1000);
}

bool MainWindow::placeRandomMove(char player, int& outR, int& outC) {
    QVector<QPair<int,int>> emptyCells;
    for (int r = 0; r < boardSize; ++r) {
        for (int c = 0; c < boardSize; ++c) {
            if (game->isCellEmpty(r, c)) emptyCells.append({r, c});
        }
    }
    if (emptyCells.isEmpty()) return false;
    auto cell = emptyCells[QRandomGenerator::global()->bounded(emptyCells.size())];
    outR = cell.first; outC = cell.second;
    return game->makeMove(outR, outC, player);
}

void MainWindow::handleTimeout() {
    if (gameOver) return;
    if (!turnTimer) return;
    remainingSeconds--;
    if (remainingSeconds > 0) {
        QString msg = QString("Ход %1 | %2 c").arg(currentPlayer).arg(remainingSeconds);
        printStatus(msg);
        return;
    }
    turnTimer->stop();
    int r = -1, c = -1;
    if (vsAI && currentPlayer == 'O') {
        if (lastXRow == -1 || lastXCol == -1) {
            placeRandomMove('X', lastXRow, lastXCol);
            updateBoard();
        }
        triggerAIMove(lastXRow, lastXCol);
        return;
    }
    if (!placeRandomMove(currentPlayer, r, c)) return;
    updateBoard();
    if (currentPlayer == 'X') { lastXRow = r; lastXCol = c; }
    if (game->checkWin(currentPlayer)) { printStatus(currentPlayer == 'X' ? "🎉 ПОБЕДИЛ X!" : "🎉 ПОБЕДИЛ O!"); return; }
    if (game->isFull()) { printStatus("НИЧЬЯ!"); return; }
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    if (vsAI && currentPlayer == 'O') {
        triggerAIMove(r, c);
    } else {
        QString msg = QString("Ход %1").arg(currentPlayer);
        startTurnTimer(msg);
        printStatus(msg);
    }
}

void MainWindow::triggerAIMove(int playerLastR, int playerLastC) {
    if (gameOver) return;
    QString threatStatus;
    bool xOneMove = isXOneMoveFromWin();
    bool xTwoMoves = isXTwoMovesFromWin();
    if (xOneMove) threatStatus = "🚨 X в 1 ходе от победы!";
    else if (xTwoMoves) threatStatus = "⚠️ X в 2 ходах от победы!";
    else threatStatus = "🧠 ИИ думает...";
    printStatus(threatStatus);
    startTurnTimer("Ход O (ИИ)");
    QTimer::singleShot(400, [this, playerLastR, playerLastC, xOneMove, xTwoMoves]() {
        if (turnTimer) turnTimer->stop();
        int bestR = -1, bestC = -1;
        int bestScore = -1000000000;
        const auto& boardSnap = game->getBoard();
        QVector<QPair<int,int>> threatPath;
        int threatCost = minMovesForXToWin(&threatPath);
        QVector<QPair<int,int>> oPath;
        int oPathCost = minMovesForOToWin(&oPath);
        QVector<QPair<int,int>> emptyCells;
        int cellCount = 0;
        for (int r = 0; r < boardSize && cellCount < 50; ++r) {
            for (int c = 0; c < boardSize && cellCount < 50; ++c) {
                if (game->isCellEmpty(r, c)) {
                    emptyCells.append({r, c});
                    cellCount++;
                }
            }
        }
        // Если X выигрывает за 1 ход, ищем любой блокирующий ход (приоритет пути угрозы).
        if (bestR == -1 && threatCost <= 1) {
            // Сначала клетки из критического пути
            for (const auto& cell : threatPath) {
                int r = cell.first, c = cell.second;
                if (!game->isCellEmpty(r, c)) continue;
                bestR = r; bestC = c; bestScore = 7'000'000;
                break;
            }
            // Если пути нет, перебираем все пустые клетки, которые ломают победу X
            if (bestR == -1) {
                for (const auto& cell : emptyCells) {
                    int r = cell.first, c = cell.second;
                    game->makeMove(r, c, 'O');
                    bool stillWin = isXOneMoveFromWin();
                    game->undoMove(r, c);
                    if (!stillWin) { bestR = r; bestC = c; bestScore = 6'800'000; break; }
                }
            }
        }
        // Жёстко перекрываем любой конкретный выигрышный ход X: если X ставит и выигрывает, ставим туда O.
        if (bestR == -1) {
            for (const auto& cell : emptyCells) {
                int r = cell.first, c = cell.second;
                game->makeMove(r, c, 'X');
                bool xWinsHere = game->checkWin('X');
                game->undoMove(r, c);
                if (xWinsHere) {
                    bestR = r; bestC = c; bestScore = 7'200'000;
                    break;
                }
            }
        }
        // Если на нижней строке есть клетка, после которой X выигрывает, блокируем её немедленно.
        if (bestR == -1) {
            for (const auto& cell : emptyCells) {
                int r = cell.first, c = cell.second;
                if (r != boardSize - 1) continue;
                game->makeMove(r, c, 'X');
                bool xWinBottom = game->checkWin('X');
                game->undoMove(r, c);
                if (xWinBottom) {
                    bestR = r; bestC = c; bestScore = 6'500'000;
                    break;
                }
            }
        }
        // Превентивно портим линию X по нижней строке, если там уже много X или путь X короткий.
        if (bestR == -1) {
            int bottomX = 0;
            for (int c = 0; c < boardSize; ++c) {
                if (boardSnap[boardSize - 1][c] == 'X') bottomX++;
            }
            if (bottomX >= boardSize / 3 || threatCost <= 3) {
                int bestRaise = -1000000000;
                int bestCenter = 1'000'000;
                for (const auto& cell : emptyCells) {
                    int r = cell.first, c = cell.second;
                    if (r != boardSize - 1) continue;
                    game->makeMove(r, c, 'O');
                    int newCost = minMovesForXToWin(nullptr);
                    game->undoMove(r, c);
                    int raise = newCost - threatCost;
                    int centerDist = abs(c - boardSize / 2);
                    if (raise > bestRaise || (raise == bestRaise && centerDist < bestCenter)) {
                        bestRaise = raise;
                        bestCenter = centerDist;
                        bestR = r; bestC = c;
                        bestScore = 5'800'000;
                    }
                }
            }
        }
        if (threatCost <= 2) {
            if (threatCost <= 1 && !threatPath.isEmpty()) {
                bestR = threatPath.front().first;
                bestC = threatPath.front().second;
                bestScore = 5'000'000;
            }
            int bestRaise = -1;
            int bestCenter = 1'000'000;
            int bestRowBias = -1;
            for (const auto& cell : emptyCells) {
                int r = cell.first, c = cell.second;
                game->makeMove(r, c, 'O');
                int newCost = minMovesForXToWin(nullptr);
                game->undoMove(r, c);
                int raise = newCost - threatCost;
                int centerDist = abs(r - boardSize / 2) + abs(c - boardSize / 2);
                if (raise > bestRaise ||
                    (raise == bestRaise && r > bestRowBias) ||
                    (raise == bestRaise && r == bestRowBias && centerDist < bestCenter)) {
                    bestRaise = raise;
                    bestCenter = centerDist;
                    bestRowBias = r;
                    bestR = r;
                    bestC = c;
                    bestScore = 4'000'000;
                }
            }
        }
        // Если путь X короткий (<=3), усиливаем перекрытие его минимального пути.
        if (bestR == -1 && threatCost <= 3 && !threatPath.isEmpty()) {
            int localBest = -1000000000;
            for (const auto& cell : threatPath) {
                int r = cell.first, c = cell.second;
                if (!game->isCellEmpty(r, c)) continue;
                game->makeMove(r, c, 'O');
                int score = evaluateMoveForO(r, c, playerLastR, playerLastC, threatCost, threatPath, oPathCost, oPath);
                game->undoMove(r, c);
                if (score > localBest) {
                    localBest = score;
                    bestR = r; bestC = c; bestScore = localBest;
                }
            }
        }
        if (bestR == -1) {
            // Первый ход ИИ — в центр или в ближайшую точку своего кратчайшего пути.
            if (aiFirstMove) {
                int center = boardSize / 2;
                if (game->isCellEmpty(center, center)) {
                    bestR = center;
                    bestC = center;
                    bestScore = 3'500'000;
                } else if (!oPath.isEmpty()) {
                    bestR = oPath.front().first;
                    bestC = oPath.front().second;
                    bestScore = 3'400'000;
                }
                aiFirstMove = false;
            }
            if (bestR == -1) {
                // Минимакс глубиной 2: O -> X -> оценка
                auto evalState = [this]() -> int {
                    int xCost = minMovesForXToWin(nullptr);
                    int oCost = minMovesForOToWin(nullptr);
                    int score = 0;
                    score += (50 - std::min(50, oCost)) * 30000;
                    score -= (50 - std::min(50, xCost)) * 32000;
                    return score;
                };
                auto buildXCands = [&](QVector<QPair<int,int>>& xs) {
                    xs.clear();
                    QVector<std::tuple<int,int,int>> tmp;
                    for (const auto& cell : emptyCells) {
                        int r = cell.first, c = cell.second;
                        int bias = 0;
                        if (r >= boardSize - 2) bias += 400;
                        if (r == boardSize - 1) bias += 800;
                        int centerDist = abs(r - boardSize/2) + abs(c - boardSize/2);
                        bias -= centerDist * 5;
                        tmp.append({bias, r, c});
                    }
                    std::sort(tmp.begin(), tmp.end(), [](auto a, auto b) { return std::get<0>(a) > std::get<0>(b); });
                    int lim = std::min<int>(12, tmp.size());
                    for (int i = 0; i < lim; ++i) xs.append({std::get<1>(tmp[i]), std::get<2>(tmp[i])});
                };

                struct MoveScore { int score; int r; int c; };
                QVector<MoveScore> oCands;
                for (const auto& cell : oPath) {
                    if (game->isCellEmpty(cell.first, cell.second))
                        oCands.append({300, cell.first, cell.second});
                }
                for (const auto& cell : emptyCells) {
                    int r = cell.first, c = cell.second;
                    int quick = 0;
                    if (r >= boardSize - 2) quick += 200;
                    if (r == boardSize - 1) quick += 400;
                    int centerDist = abs(r - boardSize/2) + abs(c - boardSize/2);
                    quick -= centerDist * 3;
                    oCands.append({quick, r, c});
                }
                std::sort(oCands.begin(), oCands.end(), [](const MoveScore& a, const MoveScore& b){ return a.score > b.score; });
                int oLim = std::min<int>(18, oCands.size());

                int globalBest = -2000000000;
                int chosenR = -1, chosenC = -1;
                QVector<QPair<int,int>> xCandidates;

                for (int idx = 0; idx < oLim; ++idx) {
                    int r = oCands[idx].r;
                    int c = oCands[idx].c;
                    if (!game->isCellEmpty(r, c)) continue;
                    game->makeMove(r, c, 'O');
                    if (game->checkWin('O')) {
                        game->undoMove(r, c);
                        bestR = r; bestC = c; bestScore = 100000000;
                        break;
                    }
                    int worstForO = 2000000000;
                    buildXCands(xCandidates);
                    if (xCandidates.isEmpty()) {
                        worstForO = evalState();
                    } else {
                        for (const auto& xc : xCandidates) {
                            int xr = xc.first, xcCol = xc.second;
                            game->makeMove(xr, xcCol, 'X');
                            if (game->checkWin('X')) {
                                worstForO = std::min(worstForO, -100000000);
                            } else {
                                int s = evalState();
                                worstForO = std::min(worstForO, s);
                            }
                            game->undoMove(xr, xcCol);
                        }
                    }
                    game->undoMove(r, c);
                    if (worstForO > globalBest) {
                        globalBest = worstForO;
                        chosenR = r; chosenC = c;
                    }
                }

                if (bestR == -1 && chosenR != -1) {
                    bestR = chosenR; bestC = chosenC; bestScore = globalBest;
                }
            }
            if (bestR == -1) {
                struct MoveScore { int score; int pos; int c; };
                QVector<MoveScore> topMoves;
                for (const auto& cell : emptyCells) {
                    int r = cell.first, c = cell.second;
                    int quickScore = 0;
                    if (abs(r - playerLastR) + abs(c - playerLastC) <= 2) quickScore += 1000;
                    if (r == 0 || r == boardSize-1) quickScore += 500;
                    topMoves.append({quickScore, r * boardSize + c, c});
                    game->makeMove(r, c, 'O');
                    if (game->checkWin('O')) {
                        bestR = r; bestC = c; bestScore = 5000000;
                        game->undoMove(r, c);
                        break;
                    }
                    game->undoMove(r, c);
                }
                int limit = std::min<int>(20, static_cast<int>(topMoves.size()));
                for (int i = 0; i < limit; ++i) {
                    int r = topMoves[i].pos / boardSize;
                    int c = topMoves[i].pos % boardSize;
                    game->makeMove(r, c, 'O');
                    int score = evaluateMoveForO(r, c, playerLastR, playerLastC, threatCost, threatPath, oPathCost, oPath);
                    game->undoMove(r, c);
                    if (score > bestScore) {
                        bestScore = score;
                        bestR = r;
                        bestC = c;
                    }
                }
            }
        }
        if (bestR == -1 && !oPath.isEmpty()) {
            int localBestScore = -1000000000;
            for (int i = 0; i < oPath.size(); ++i) {
                int r = oPath[i].first;
                int c = oPath[i].second;
                if (!game->isCellEmpty(r, c)) continue;
                game->makeMove(r, c, 'O');
                int score = evaluateMoveForO(r, c, playerLastR, playerLastC, threatCost, threatPath, oPathCost, oPath);
                game->undoMove(r, c);
                if (score > localBestScore) {
                    localBestScore = score;
                    bestR = r;
                    bestC = c;
                }
            }
            if (bestR != -1) bestScore = localBestScore;
        }
        if (bestR != -1) {
            game->makeMove(bestR, bestC, 'O');
            aiFirstMove = false;
            updateBoard();
            QString moveMsg;
            if (xOneMove || xTwoMoves) {
                moveMsg = QString("🛡️ Блокируем победу X: [%1,%2]").arg(bestR).arg(bestC);
            } else {
                moveMsg = QString("🔗 ИИ строит путь: [%1,%2]").arg(bestR).arg(bestC);
            }
            if (game->checkWin('O')) {
                finishGame("🤖 ПОБЕДИЛ ИИ O!");
                return;
            }
            if (game->isFull()) {
                finishGame("НИЧЬЯ!");
                return;
            }
            currentPlayer = 'X';
            QString msg = "Твой ход X";
            printStatus(msg);
            startTurnTimer(msg);
        }
    });
}

void MainWindow::finishGame(const QString& winnerText) {
    gameOver = true;
    if (turnTimer) turnTimer->stop();
    printStatus(winnerText);
    QMessageBox box(this);
    box.setWindowTitle("Игра завершена");
    box.setText(winnerText);
    QPushButton* newGameBtn = box.addButton("Новая игра", QMessageBox::AcceptRole);
    QPushButton* exitBtn = box.addButton("Выход", QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == newGameBtn) {
        newGame();
    } else if (box.clickedButton() == exitBtn) {
        close();
    }
}

void MainWindow::onCellClicked(int row, int col) {
    if (gameOver) return;
    if (turnTimer) turnTimer->stop();
    if (currentPlayer != 'X' && vsAI) {
        startTurnTimer(QString("Ход %1").arg(currentPlayer));
        return;
    }
    if (currentPlayer != 'X' && !vsAI && currentPlayer != 'O') {
        startTurnTimer(QString("Ход %1").arg(currentPlayer));
        return;
    }
    if (!game->makeMove(row, col, currentPlayer)) {
        if (turnTimer) startTurnTimer(QString("Ход %1").arg(currentPlayer));
        return;
    }
    updateBoard();
    if (currentPlayer == 'X') { lastXRow = row; lastXCol = col; }
    if (game->checkWin(currentPlayer)) {
        finishGame(currentPlayer == 'X' ? "🎉 ПОБЕДИЛ X!" : "🎉 ПОБЕДИЛ O!");
        return;
    }
    if (game->isFull()) {
        finishGame("НИЧЬЯ!");
        return;
    }
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    if (vsAI && currentPlayer == 'O') {
        triggerAIMove(row, col);
    } else {
        QString msg = QString("Ход %1").arg(currentPlayer);
        printStatus(msg);
        startTurnTimer(msg);
    }
}

int MainWindow::evaluateMoveForO(int r, int c, int playerLastR, int playerLastC,
                                 int baseThreatCost,
                                 const QVector<QPair<int,int>>& threatPath,
                                 int baseOPathCost,
                                 const QVector<QPair<int,int>>& oPath) {
    const auto &board = game->getBoard();
    int size = game->getSize();
    int score = 0;
    if (game->checkWin('O')) return 5000000;
    int newThreat = minMovesForXToWin(nullptr);
    int threatDelta = baseThreatCost - newThreat;
    if (newThreat <= 1) score += 800000;
    if (threatDelta > 0) score += threatDelta * 400000;
    for (const auto& cell : threatPath) {
        if (cell.first == r && cell.second == c) {
            score += 180000;
            break;
        }
    }
    int newOPathCost = minMovesForOToWin(nullptr);
    int oGain = baseOPathCost - newOPathCost;
    if (newOPathCost <= 1) score += 700000;
    if (oGain > 0) score += oGain * 300000;
    for (const auto& cell : oPath) {
        if (cell.first == r && cell.second == c) {
            score += 250000; // бонус за продвижение по своему кратчайшему пути
            break;
        }
    }
    game->makeMove(r, c, 'X');
    bool xWinHere = game->checkWin('X');
    game->undoMove(r, c);
    if (xWinHere) return 3000000;
    bool xOneThreat = isXOneMoveFromWin();
    if (xOneThreat) score += 2500000;
    else if (isXTwoMovesFromWin()) score += 2000000;
    int pathScore = shortestPathToConnectO(r, c);
    score += (size * 3 - pathScore) * 8000;
    if (r >= size - 2) score += 120000; // агрессивно блокируем низ поля
    if (playerLastR >= size - 2 && abs(r - playerLastR) <= 1) score += 120000;
    int distToPlayer = abs(r - playerLastR) + abs(c - playerLastC);
    if (distToPlayer <= 2) score += (3 - distToPlayer) * 10000;
    if (c >= size - 3) score += 8000;
    if (r <= 1 || r >= size - 2) score += 5000;
    int neighbors = 0;
    for (int d = 0; d < 6; ++d) {
        int nr = r + kHexDirections[d][0];
        int nc = c + kHexDirections[d][1];
        if (nr >= 0 && nr < size && nc >= 0 && nc < size) {
            if (board[nr][nc] == 'O') neighbors += 2;
            if (board[nr][nc] == 'X') neighbors += 1;
        }
    }
    score += neighbors * 1000;
    int centerDist = abs(r - size/2) + abs(c - size/2);
    score += (size - centerDist) * 200;
    return score;
}

void MainWindow::updateBoard() {
    const auto board = game->getBoard();
    for (int r = 0; r < boardSize; ++r) {
        for (int c = 0; c < boardSize; ++c) {
            QPushButton* btn = buttons[r][c];
            char cell = board[r][c];
            if (cell == 'X') {
                btn->setText("X");
                btn->setStyleSheet(
                    "QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #FF8888, stop:1 #CC0000); "
                    "border: 2px solid #FF4444; border-radius: 24px; font-size: 22px; font-weight: bold; color: white; }");
            } else if (cell == 'O') {
                btn->setText("O");
                btn->setStyleSheet(
                    "QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #88CCFF, stop:1 #0055CC); "
                    "border: 2px solid #44AAFF; border-radius: 24px; font-size: 22px; font-weight: bold; color: white; }");
            } else {
                btn->setText("");
                btn->setStyleSheet(
                    "QPushButton { border: 2px solid #777; border-radius: 24px; "
                    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #DDDDDD, stop:1 #AAAAAA); "
                    "font-size: 22px; font-weight: bold; }"
                    "QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #EEEEEE, stop:1 #CCCCCC); }");
            }
        }
    }
}

void MainWindow::printStatus(const QString &message) {
    statusLabel->setText(message);
}

void MainWindow::showMenu() {
    QMessageBox::information(this, "Супер ИИ HEX",
                             "Смысл игры\n"
                             "Гекс (Hex) — соединить противоположные стороны игрового поля непрерывной цепочкой своих фишек, блокируя соперника, который стремится сделать то же самое между своими сторонами, при этом игра не допускает ничьих, развивая стратегическое и логическое мышление, память, моторику и реакцию.\n\n"
                             "ИИ анализирует угрозы и строит кратчайший путь для O.\n"
                             "Блокирует победу X, старается выиграть сам.\n"
                             "Рандомность минимальна, ходы продуманные.");
}
