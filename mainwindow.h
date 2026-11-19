#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include "fighter.h"
#include "gameui.h"
#include "playerlogic.h"

namespace Ui { class MainWindow; }

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum class GameState { Menu, Playing, GameOver };

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

private slots:
    void gameLoop();

private:
    void physicsUpdate(Fighter &f);
    bool aabbIntersect(const QRect &a, const QRect &b) const;
    void resetRound();
    void checkWin();
    void updateUiWidgets();

    Ui::MainWindow *ui;
    QTimer *timer = nullptr;

    Fighter player1Graphics;
    Fighter player2Graphics;
    GameUI gameUI;
    PlayerLogic player1Logic;
    PlayerLogic player2Logic;

    GameState m_state = GameState::Menu;

    bool p1Left=false,p1Right=false,p2Left=false,p2Right=false;

    static constexpr double GRAVITY  = 0.9;
    static constexpr double JUMP_V   = -16.0;
    static constexpr double MOVE_SPD = 4.5;
    static constexpr int    GROUND_Y = 500;
    static constexpr int    ROUND_TIME = 60;

    int m_timerAccum = 0;
};

#endif // MAINWINDOW_H