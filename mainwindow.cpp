#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    resize(800, 600);
    setFocusPolicy(Qt::StrongFocus);

    player1Logic.attach(&player1Graphics);
    player2Logic.attach(&player2Graphics);

    player1Graphics.placeholderColor = Qt::darkRed;
    player2Graphics.placeholderColor = Qt::darkBlue;

    // 组员A：素材就绪后取消注释并填真实路径/帧数/尺寸
    // player1Graphics.loadFrames(AnimState::Idle,  ":/res/p1_idle.png",  4, 80, 120);
    // player1Graphics.loadFrames(AnimState::Walk,  ":/res/p1_walk.png",  6, 80, 120);
    // player1Graphics.loadFrames(AnimState::Jump,  ":/res/p1_jump.png",  3, 80, 120);
    // player1Graphics.loadFrames(AnimState::Attack,":/res/p1_attack.png",5, 80, 120);
    // player1Graphics.loadFrames(AnimState::Hurt,  ":/res/p1_hurt.png",  2, 80, 120);

    player1Graphics.reset(150, GROUND_Y - 120);
    player2Graphics.reset(550, GROUND_Y - 120);
    player2Graphics.facingRight = false;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(16);

    updateUiWidgets();
}

MainWindow::~MainWindow() { delete ui; }

bool MainWindow::aabbIntersect(const QRect &a, const QRect &b) const {
    return a.intersects(b);
}

void MainWindow::resetRound() {
    player1Graphics.maxHp = 100;
    player2Graphics.maxHp = 100;
    player1Graphics.reset(150, GROUND_Y - 120);
    player2Graphics.reset(550, GROUND_Y - 120);
    player1Graphics.facingRight = true;
    player2Graphics.facingRight = false;
    gameUI.resetTimer(ROUND_TIME);
    m_timerAccum = 0;
    p1Left=p1Right=p2Left=p2Right=false;
    ui->koLabel->hide();
}

void MainWindow::physicsUpdate(Fighter &f) {
    f.vy += GRAVITY;
    f.x  += f.vx;
    f.y  += f.vy;

    if (f.x < 0) f.x = 0;
    if (f.x + f.getBodyBox().width() > width())
        f.x = width() - f.getBodyBox().width();

    int bottom = f.y + f.getBodyBox().height();
    if (bottom >= GROUND_Y) {
        f.y = GROUND_Y - f.getBodyBox().height();
        f.vy = 0;
        f.onGround = true;
    } else {
        f.onGround = false;
    }
}

void MainWindow::checkWin() {
    if (m_state != GameState::Playing) return;
    if (player1Graphics.hp <= 0 || player2Graphics.hp <= 0
        || gameUI.getTimeLeft() <= 0) {
        m_state = GameState::GameOver;
        QString msg;
        if (player1Graphics.hp <= 0 && player2Graphics.hp <= 0) msg = "平局！";
        else if (player2Graphics.hp <= 0) msg = "玩家1 胜利！";
        else if (player1Graphics.hp <= 0) msg = "玩家2 胜利！";
        else msg = "时间到！";
        ui->koLabel->setText(msg);
        ui->koLabel->show();
    }
}

void MainWindow::updateUiWidgets() {
    ui->p1Bar->setValue(player1Graphics.hp);
    ui->p2Bar->setValue(player2Graphics.hp);
    ui->menuLabel->setText(QString::number(gameUI.getTimeLeft()));
    ui->menuLabel->setVisible(m_state == GameState::Menu);
}

void MainWindow::gameLoop() {
    if (m_state == GameState::Playing) {
        player1Graphics.vx = (p1Right - p1Left) * MOVE_SPD;
        player2Graphics.vx = (p2Right - p2Left) * MOVE_SPD;

        physicsUpdate(player1Graphics);
        physicsUpdate(player2Graphics);
        player1Graphics.faceTo(player2Graphics);
        player2Graphics.faceTo(player1Graphics);

        player1Logic.update();
        player2Logic.update();

        if (aabbIntersect(player1Graphics.getHitBox(), player2Graphics.getBodyBox()))
            player1Logic.checkHit(true, &player2Graphics);
        if (aabbIntersect(player2Graphics.getHitBox(), player1Graphics.getBodyBox()))
            player2Logic.checkHit(true, &player1Graphics);

        player1Graphics.updateAnimation();
        player2Graphics.updateAnimation();

        if (++m_timerAccum >= 60) { m_timerAccum = 0; gameUI.tickTimer(); }
        checkWin();
        updateUiWidgets();
    }
    update();
}

void MainWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(30, 30, 40));
    painter.fillRect(0, GROUND_Y, width(), height() - GROUND_Y, QColor(70, 60, 50));
    player1Graphics.draw(painter);
    player2Graphics.draw(painter);
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (m_state == GameState::Menu && e->key() == Qt::Key_Space) {
        resetRound();
        m_state = GameState::Playing;
        updateUiWidgets();
        return;
    }
    if (m_state == GameState::GameOver && e->key() == Qt::Key_Space) {
        m_state = GameState::Menu;
        updateUiWidgets();
        return;
    }
    if (m_state != GameState::Playing) return;

    switch (e->key()) {
    case Qt::Key_A: p1Left = true; break;
    case Qt::Key_D: p1Right = true; break;
    case Qt::Key_W:
        if (player1Graphics.onGround) { player1Graphics.vy = JUMP_V; player1Graphics.onGround = false; }
        break;
    case Qt::Key_J: player1Logic.attack(); break;

    case Qt::Key_Left:  p2Left = true; break;
    case Qt::Key_Right: p2Right = true; break;
    case Qt::Key_Up:
        if (player2Graphics.onGround) { player2Graphics.vy = JUMP_V; player2Graphics.onGround = false; }
        break;
    case Qt::Key_1: player2Logic.attack(); break;
    default: break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *e) {
    switch (e->key()) {
    case Qt::Key_A: p1Left = false; break;
    case Qt::Key_D: p1Right = false; break;
    case Qt::Key_Left:  p2Left = false; break;
    case Qt::Key_Right: p2Right = false; break;
    default: break;
    }
}