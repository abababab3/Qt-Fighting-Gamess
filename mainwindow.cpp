#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QDir>
#include <QFont>
#include <QLinearGradient>
#include <windows.h>
#include <mmsystem.h>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    resize(800, 600);
    setWindowTitle("格斗小游戏 · Fighting Game");
    setFocusPolicy(Qt::StrongFocus);

    player1Logic.attach(&player1Graphics);
    player2Logic.attach(&player2Graphics);

    player1Graphics.placeholderColor = Qt::darkRed;
    player2Graphics.placeholderColor = Qt::darkBlue;

    // ===== 玩家1：每帧 200×200，帧数自动按宽度推导 =====
    player1Graphics.loadFrames(AnimState::Idle,   ":/res/p1_ldle.png",    8, 200, 200);
    player1Graphics.loadFrames(AnimState::Walk,   ":/res/p1_walk.png",    8, 200, 200);
    player1Graphics.loadFrames(AnimState::Jump,   ":/res/p1_jump.png",    2, 200, 200);
    player1Graphics.loadFrames(AnimState::Attack,  ":/res/p1_attack1.png", 6, 200, 200);
    player1Graphics.loadFrames(AnimState::Attack2, ":/res/p1_attack2.png", 6, 200, 200);
    player1Graphics.loadFrames(AnimState::Hurt,   ":/res/p1_hurt.png",    4, 200, 200);

    // ===== 玩家2：每帧 180×180 =====
    player2Graphics.loadFrames(AnimState::Idle,   ":/res/p2_ldle.png",    11, 180, 180);
    player2Graphics.loadFrames(AnimState::Walk,   ":/res/p2_walk.png",     8, 180, 180);
    player2Graphics.loadFrames(AnimState::Jump,   ":/res/p2_jump.png",     3, 180, 180);
    player2Graphics.loadFrames(AnimState::Attack,  ":/res/p2_attack1.png",  7, 180, 180);
    player2Graphics.loadFrames(AnimState::Attack2, ":/res/p2_attack2.png",  7, 180, 180);
    player2Graphics.loadFrames(AnimState::Hurt,   ":/res/p2_hurt.png",     4, 180, 180);

    player1Graphics.reset(100, GROUND_Y - 200);
    player2Graphics.reset(600, GROUND_Y - 180);
    player1Graphics.setFacing(true);
    player2Graphics.setFacing(false);

    ui->menuLabel->hide();
    ui->koLabel->hide();

    initAudio();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(16);

    updateUiWidgets();
}

MainWindow::~MainWindow() {
    mciSendStringA("stop bgm", 0, 0, 0);
    mciSendStringA("close bgm", 0, 0, 0);
    delete ui;
}

// ===== 初始化音频：把资源里的 wav 释放到临时目录，然后循环播放 BGM =====
void MainWindow::initAudio() {
    const char *names[] = {"bgm", "hit", "jump", "ko"};
    QString dir = QDir::tempPath();
    for (const char *n : names) {
        QString res = QString(":/audio/%1.wav").arg(n);
        QString dst = dir + QString("/qtfg_%1.wav").arg(n);
        QFile src(res);
        if (src.open(QIODevice::ReadOnly)) {
            QFile::remove(dst);
            QFile out(dst);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(src.readAll());
                out.close();
            }
            src.close();
        }
    }
    QString bgm = dir + "/qtfg_bgm.wav";
    QByteArray cmd = QString("open \"%1\" type waveaudio alias bgm").arg(bgm).toLocal8Bit();
    mciSendStringA(cmd.constData(), 0, 0, 0);
    mciSendStringA("play bgm repeat", 0, 0, 0);
}

// ===== 播放音效（waveaudio 一次一个，用独占 alias，先关再开）=====
void MainWindow::playSfx(const char *resName, const char *alias) {
    QString path = QDir::tempPath() + QString("/qtfg_%1.wav").arg(resName);
    QByteArray closeCmd = QString("close %1").arg(alias).toLocal8Bit();
    mciSendStringA(closeCmd.constData(), 0, 0, 0);
    QByteArray openCmd = QString("open \"%1\" type waveaudio alias %2").arg(path, alias).toLocal8Bit();
    mciSendStringA(openCmd.constData(), 0, 0, 0);
    QByteArray playCmd = QString("play %1").arg(alias).toLocal8Bit();
    mciSendStringA(playCmd.constData(), 0, 0, 0);
}

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
        if (player1Graphics.hp <= 0 && player2Graphics.hp <= 0)      m_resultMsg = "平局！";
        else if (player2Graphics.hp <= 0)                            m_resultMsg = "玩家1 胜利！";
        else if (player1Graphics.hp <= 0)                            m_resultMsg = "玩家2 胜利！";
        else                                                         m_resultMsg = "时间到！";
        playSfx("ko", "koalias");
    }
}

void MainWindow::updateUiWidgets() {
    ui->p1Bar->setValue(player1Graphics.hp);
    ui->p2Bar->setValue(player2Graphics.hp);
    ui->timerLable->setText(QString::number(gameUI.getTimeLeft()));
    // 血条和计时只在游戏进行时显示，菜单/结算由自绘画面接管
    bool inGame = (m_state == GameState::Playing);
    ui->p1Bar->setVisible(inGame);
    ui->p2Bar->setVisible(inGame);
    ui->timerLable->setVisible(inGame);
    ui->menuLabel->hide();
    ui->koLabel->hide();
}

void MainWindow::gameLoop() {
    ++m_tick;
    if (m_state == GameState::Playing) {
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

        QRect p1Hit = player1Graphics.getHitBox();
        if (!p1Hit.isNull() && aabbIntersect(p1Hit, player2Graphics.getBodyBox())) {
            player1Logic.checkHit(true, &player2Logic);
            playSfx("hit", "hitalias");
        }

        QRect p2Hit = player2Graphics.getHitBox();
        if (!p2Hit.isNull() && aabbIntersect(p2Hit, player1Graphics.getBodyBox())) {
            player2Logic.checkHit(true, &player1Logic);
            playSfx("hit", "hitalias2");
        }

        player1Graphics.updateAnimation();
        player2Graphics.updateAnimation();

        if (player1Graphics.flashTicks > 0) --player1Graphics.flashTicks;
        if (player2Graphics.flashTicks > 0) --player2Graphics.flashTicks;

        if (++m_timerAccum >= 60) { m_timerAccum = 0; gameUI.tickTimer(); }
        checkWin();
        updateUiWidgets();
    }
    update();
}

// ===== 开始界面 =====
void MainWindow::drawMenu(QPainter &p) {
    QLinearGradient g(0, 0, 0, height());
    g.setColorAt(0,   QColor(18, 16, 38));
    g.setColorAt(0.6, QColor(48, 26, 66));
    g.setColorAt(1,   QColor(92, 40, 58));
    p.fillRect(rect(), g);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 210, 120, 22));
    p.drawEllipse(QPoint(width()/2, 210), 250, 180);
    p.setBrush(QColor(255, 120, 90, 18));
    p.drawEllipse(QPoint(width()/2, 210), 170, 120);

    QFont title = p.font();
    title.setPointSize(52); title.setBold(true);
    p.setFont(title);
    p.setPen(QColor(255, 220, 130));
    p.drawText(QRect(0, 90, width(), 90), Qt::AlignCenter, "格 斗 小 游 戏");

    QFont sub = p.font();
    sub.setPointSize(18); sub.setBold(false);
    p.setFont(sub);
    p.setPen(QColor(220, 205, 235));
    p.drawText(QRect(0, 178, width(), 40), Qt::AlignCenter, "F I G H T I N G   G A M E");

    QRect panel(140, 250, 520, 150);
    p.setBrush(QColor(10, 10, 20, 150));
    p.setPen(QPen(QColor(255, 220, 130, 160), 1));
    p.drawRoundedRect(panel, 12, 12);

    QFont info = p.font();
    info.setPointSize(14); info.setBold(false);
    p.setFont(info);
    p.setPen(QColor(255, 150, 120));
    p.drawText(QRect(160, 262, 240, 30), Qt::AlignLeft, "玩家 1");
    p.setPen(QColor(130, 190, 255));
    p.drawText(QRect(400, 262, 240, 30), Qt::AlignLeft, "玩家 2");

    p.setPen(QColor(225, 225, 235));
    p.drawText(QRect(160, 296, 240, 24), Qt::AlignLeft, "A / D 移动   W 跳跃");
    p.drawText(QRect(160, 326, 240, 24), Qt::AlignLeft, "J 攻击 (两段连招)");
    p.drawText(QRect(400, 296, 240, 24), Qt::AlignLeft, "← / → 移动   ↑ 跳跃");
    p.drawText(QRect(400, 326, 240, 24), Qt::AlignLeft, "小键盘 1 攻击 (两段连招)");
    p.setPen(QColor(180, 180, 195));
    p.drawText(panel.adjusted(0,0,0,-6), Qt::AlignHCenter | Qt::AlignBottom, "空中也可以攻击");

    if ((m_tick / 30) % 2 == 0) {
        QFont blink = p.font();
        blink.setPointSize(20); blink.setBold(true);
        p.setFont(blink);
        p.setPen(QColor(255, 235, 160));
        p.drawText(QRect(0, 450, width(), 50), Qt::AlignCenter, "按  空格键  开始游戏");
    }
}

// ===== 游戏场景：美化背景 + 地面 + 角色 =====
void MainWindow::drawScene(QPainter &p) {
    QLinearGradient sky(0, 0, 0, GROUND_Y);
    sky.setColorAt(0,   QColor(26, 30, 62));
    sky.setColorAt(0.65,QColor(58, 42, 82));
    sky.setColorAt(1,   QColor(128, 70, 72));
    p.fillRect(0, 0, width(), GROUND_Y, sky);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 235, 190, 210));
    p.drawEllipse(QRect(640, 60, 70, 70));
    p.setBrush(QColor(26, 30, 62, 90));
    p.drawEllipse(QRect(662, 52, 70, 70));

    p.setBrush(QColor(20, 18, 38, 180));
    int skyline[] = {40, 90, 55, 120, 70, 100, 45, 130, 80, 60, 110, 50};
    int sx = 0;
    for (int i = 0; i < 12; ++i) {
        int bw = 60, bh = skyline[i];
        p.drawRect(sx, GROUND_Y - bh, bw, bh);
        sx += bw;
    }

    QLinearGradient ground(0, GROUND_Y, 0, height());
    ground.setColorAt(0, QColor(110, 82, 62));
    ground.setColorAt(1, QColor(48, 36, 30));
    p.fillRect(QRect(0, GROUND_Y, width(), height() - GROUND_Y), ground);
    p.setPen(QPen(QColor(230, 190, 130, 200), 3));
    p.drawLine(0, GROUND_Y, width(), GROUND_Y);

    auto drawShadow = [&](const Fighter &f) {
        QRect b = f.getBodyBox();
        int air = GROUND_Y - b.bottom();
        if (air < 0) air = 0;
        double scale = 1.0 - qMin(air, 120) / 200.0;
        if (scale < 0.25) scale = 0.25;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, (int)(100 * scale)));
        p.drawEllipse(QPointF(b.center().x(), GROUND_Y + 5),
                      static_cast<double>(b.width()) / 3 * scale, 10.0 * scale);
    };
    drawShadow(player1Graphics);
    drawShadow(player2Graphics);

    player1Graphics.draw(p);
    player2Graphics.draw(p);

    QFont tag = p.font();
    tag.setPointSize(13); tag.setBold(true);
    p.setFont(tag);
    p.setPen(QColor(255, 150, 120));
    p.drawText(QRect(90, 6, 216, 22), Qt::AlignCenter, "玩家 1");
    p.setPen(QColor(130, 190, 255));
    p.drawText(QRect(494, 6, 216, 22), Qt::AlignCenter, "玩家 2");

    if (m_state == GameState::GameOver) {
        p.fillRect(rect(), QColor(0, 0, 0, 120));
        QFont big = p.font();
        big.setPointSize(48); big.setBold(true);
        p.setFont(big);
        p.setPen(Qt::yellow);
        p.drawText(QRect(0, 230, width(), 80), Qt::AlignCenter, m_resultMsg);

        QFont tip = p.font();
        tip.setPointSize(18); tip.setBold(false);
        p.setFont(tip);
        p.setPen(QColor(230, 230, 240));
        p.drawText(QRect(0, 320, width(), 50), Qt::AlignCenter, "按 空格键 返回标题");
    }
}

void MainWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_state == GameState::Menu)
        drawMenu(painter);
    else
        drawScene(painter);
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (e->isAutoRepeat()) return;

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
        if (player1Graphics.onGround) {
            player1Graphics.vy = JUMP_V;
            player1Graphics.onGround = false;
            playSfx("jump", "jumpalias");
        }
        break;
    case Qt::Key_J: player1Logic.attack(); break;

    case Qt::Key_Left:  p2Left = true; break;
    case Qt::Key_Right: p2Right = true; break;
    case Qt::Key_Up:
        if (player2Graphics.onGround) {
            player2Graphics.vy = JUMP_V;
            player2Graphics.onGround = false;
            playSfx("jump", "jumpalias2");
        }
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
