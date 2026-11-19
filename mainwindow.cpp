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

    // ===== 玩家1：每帧 200×200，帧数 = 图里人数（loadFrames 会自动按宽度推导） =====
    player1Graphics.loadFrames(AnimState::Idle,   ":/res/p1_ldle.png",    8, 200, 200);
    player1Graphics.loadFrames(AnimState::Walk,   ":/res/p1_walk.png",    8, 200, 200);
    player1Graphics.loadFrames(AnimState::Jump,   ":/res/p1_jump.png",    2, 200, 200);
    // P1 两组攻击：每次按 J 交替播放
    player1Graphics.loadFrames(AnimState::Attack,  ":/res/p1_attack1.png", 6, 200, 200);
    player1Graphics.loadFrames(AnimState::Attack2, ":/res/p1_attack2.png", 6, 200, 200);
    player1Graphics.loadFrames(AnimState::Hurt,   ":/res/p1_hurt.png",    4, 200, 200);

    // ===== 玩家2：每帧 180×180 =====
    player2Graphics.loadFrames(AnimState::Idle,   ":/res/p2_ldle.png",    11, 180, 180);
    player2Graphics.loadFrames(AnimState::Walk,   ":/res/p2_walk.png",     8, 180, 180);
    player2Graphics.loadFrames(AnimState::Jump,   ":/res/p2_jump.png",     3, 180, 180);
    // P2 两组攻击：每次按 1 交替播放
    player2Graphics.loadFrames(AnimState::Attack,  ":/res/p2_attack1.png", 7, 180, 180);
    player2Graphics.loadFrames(AnimState::Attack2, ":/res/p2_attack2.png", 7, 180, 180);
    player2Graphics.loadFrames(AnimState::Hurt,   ":/res/p2_hurt.png",     4, 180, 180);

    player1Graphics.reset(100, GROUND_Y - 200);
    player2Graphics.reset(600, GROUND_Y - 180);
    player1Graphics.setFacing(true);
    player2Graphics.setFacing(false);

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
    player1Graphics.reset(100, GROUND_Y - 200);
    player2Graphics.reset(600, GROUND_Y - 180);
    player1Graphics.setFacing(true);
    player2Graphics.setFacing(false);
    player1Graphics.flashTicks = player2Graphics.flashTicks = 0;
    gameUI.resetTimer(ROUND_TIME);
    m_timerAccum = 0;
    p1Left = p1Right = p2Left = p2Right = false;
    ui->koLabel->hide();
}

void MainWindow::physicsUpdate(Fighter &f) {
    f.vy += GRAVITY;
    f.x  += f.vx;
    f.y  += f.vy;

    if (f.x < 0) f.x = 0;
    if (f.x + f.getFrameWidth() > width())
        f.x = width() - f.getFrameWidth();

    QRect body = f.getBodyBox();
    if (body.bottom() >= GROUND_Y) {
        f.y += GROUND_Y - body.bottom();
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
    // UI 文件里控件名是 timerLable（少个 e）
    ui->timerLable->setText(QString::number(gameUI.getTimeLeft()));
    ui->menuLabel->setVisible(m_state == GameState::Menu);
}

void MainWindow::gameLoop() {
    if (m_state == GameState::Playing) {
        // ===== 朝向由最后的移动方向决定（自由转向，不再强制面对对手） =====
        if (p1Right && !p1Left) player1Graphics.setFacing(true);
        if (p1Left  && !p1Right) player1Graphics.setFacing(false);
        if (p2Right && !p2Left) player2Graphics.setFacing(true);
        if (p2Left  && !p2Right) player2Graphics.setFacing(false);

        player1Graphics.vx = (p1Right - p1Left) * MOVE_SPD;
        player2Graphics.vx = (p2Right - p2Left) * MOVE_SPD;

        physicsUpdate(player1Graphics);
        physicsUpdate(player2Graphics);

        player1Logic.update();
        player2Logic.update();

        // ===== 命中判定：攻击框非空 + 与对方身体框相交（空中同样生效） =====
        QRect p1Hit = player1Graphics.getHitBox();
        if (!p1Hit.isNull() && aabbIntersect(p1Hit, player2Graphics.getBodyBox()))
            player1Logic.checkHit(true, &player2Logic);

        QRect p2Hit = player2Graphics.getHitBox();
        if (!p2Hit.isNull() && aabbIntersect(p2Hit, player1Graphics.getBodyBox()))
            player2Logic.checkHit(true, &player1Logic);

        player1Graphics.updateAnimation();
        player2Graphics.updateAnimation();

        // 闪白计时递减
        if (player1Graphics.flashTicks > 0) --player1Graphics.flashTicks;
        if (player2Graphics.flashTicks > 0) --player2Graphics.flashTicks;

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

    // 人物阴影：跳起来时阴影变小变淡，增强层次感
    auto drawShadow = [&](const Fighter &f) {
        QRect b = f.getBodyBox();
        int air = GROUND_Y - b.bottom();
        if (air < 0) air = 0;
        double scale = 1.0 - qMin(air, 120) / 200.0;
        if (scale < 0.25) scale = 0.25;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, (int)(90 * scale)));
        painter.drawEllipse(QPointF(b.center().x(), GROUND_Y + 4),
                            static_cast<double>(b.width()) / 3 * scale, 10.0 * scale);
    };
    drawShadow(player1Graphics);
    drawShadow(player2Graphics);

    player1Graphics.draw(painter);
    player2Graphics.draw(painter);
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (e->isAutoRepeat()) return;   // 忽略长按重复，跳跃/攻击只触发一次

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
    case Qt::Key_J: player1Logic.attack(); break;   // 空中也能按 J 攻击

    case Qt::Key_Left:  p2Left = true; break;
    case Qt::Key_Right: p2Right = true; break;
    case Qt::Key_Up:
        if (player2Graphics.onGround) { player2Graphics.vy = JUMP_V; player2Graphics.onGround = false; }
        break;
    case Qt::Key_1: player2Logic.attack(); break;   // 空中也能按 1 攻击
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
