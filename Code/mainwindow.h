#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class HexGame;
class QLabel;
class QGridLayout;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCellClicked(int row, int col);

private:
    void setupUI();
    void newGame();
    void showIntro();
    void showMenu();
    void updateBoard();
    void printStatus(const QString &message);
    void startTurnTimer(const QString& baseStatus);
    void handleTimeout();
    bool placeRandomMove(char player, int& outR, int& outC);
    void triggerAIMove(int playerLastR, int playerLastC);
    void finishGame(const QString& winnerText);
    int evaluateMoveForO(int r, int c, int playerLastR, int playerLastC,
                         int baseThreatCost,
                         const QVector<QPair<int,int>>& threatPath,
                         int baseOPathCost,
                         const QVector<QPair<int,int>>& oPath);
    int minMovesForXToWin(QVector<QPair<int,int>>* path = nullptr);
    int minMovesForOToWin(QVector<QPair<int,int>>* path = nullptr);

    // 🆕 НОВЫЕ ФУНКЦИИ ДЛЯ СУПЕР ИИ
    bool isXOneMoveFromWin();
    bool isXTwoMovesFromWin();
    int shortestPathToConnectO(int r, int c);

    Ui::MainWindow *ui;
    HexGame* game;
    int boardSize;
    char currentPlayer;
    bool vsAI;
    QLabel* statusLabel;
    QGridLayout* gameGrid;
    std::vector<std::vector<QPushButton*>> buttons;
    bool aiFirstMove;
    QTimer* turnTimer;
    int remainingSeconds;
    int lastXRow;
    int lastXCol;
    bool gameOver;
};

#endif // MAINWINDOW_H
