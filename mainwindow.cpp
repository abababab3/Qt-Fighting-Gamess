#include "mainwindow.h"
#include "backgroundmusic.h"
#include "keysettingsdialog.h"

#include <QAction>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QShortcut>
#include <QSlider>
#include <QTimer>
#include <algorithm>
#include <windows.h>
#include <mmsystem.h>

namespace {

QPushButton *makeButton(QWidget *parent, const QString &text, bool primary = false)
{
    auto *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setProperty("primary", primary);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { background: #29242f; color: #f6ebda;"
        " border: 1px solid #5c4c51; border-radius: 8px; }"
        "QPushButton:hover { background: #392e38; border-color: #947365; }"
        "QPushButton:pressed { background: #211b26; }"
        "QPushButton:checked { background: #1d1923; color: #c9b8aa; }"
        "QPushButton[primary=\"true\"] { background: #edb578; color: #251915;"
        " border: 1px solid #edb578; }"
        "QPushButton[primary=\"true\"]:hover { background: #ffd2a0; }"
        "QPushButton[primary=\"true\"]:pressed { background: #c98e56; }"
        "QPushButton[mode=\"true\"] { text-align: left; padding-left: 17px; }"
        "QPushButton:focus, QPushButton[primary=\"true\"]:focus { border: 2px solid #a9e0df; }"));
    return button;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      localButton(makeButton(this, QStringLiteral("本地双人对战"), true)),
      computerButton(makeButton(this, QStringLiteral("与电脑对战"))),
      trainingButton(makeButton(this, QStringLiteral("木桩练习"))),
      keySettingsButton(makeButton(this, QStringLiteral("键位设置"))),
      retryButton(makeButton(this, QStringLiteral("再战一局"), true)),
      menuButton(makeButton(this, QStringLiteral("返回主菜单"))),
      musicMuteButton(makeButton(this, QString())),
      musicVolumeSlider(new QSlider(Qt::Horizontal, this)),
      stageSelector(makeButton(this, QString())),
      stageMenu(new QMenu(stageSelector)),
      backgroundMusic(new BackgroundMusic(this)),
      player1Logic(&player1Graphics, SkillKind::SwordWave),
      player2Logic(&player2Graphics, SkillKind::Lightning)
{
    setMinimumSize(480, 360);
    resize(GameUI::Width, GameUI::Height);
    setWindowTitle(QStringLiteral("格斗竞技场"));
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    // 选择面板是自绘的，要靠鼠标移动做悬停反馈
    setMouseTracking(true);

    for (auto *button : {localButton, computerButton, trainingButton, keySettingsButton})
        button->setProperty("mode", true);

    computerButton->setAccessibleDescription(QStringLiteral("玩家 1 对战电脑"));
    connect(localButton, &QPushButton::clicked, this, &MainWindow::startLocalMatch);
    connect(computerButton, &QPushButton::clicked, this, &MainWindow::startComputerMatch);
    connect(trainingButton, &QPushButton::clicked, this, &MainWindow::startTrainingMatch);
    connect(keySettingsButton, &QPushButton::clicked, this, &MainWindow::openKeySettings);
    connect(retryButton, &QPushButton::clicked, this, &MainWindow::startRound);
    connect(menuButton, &QPushButton::clicked, this, &MainWindow::returnToMenu);

    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escape->setContext(Qt::WidgetWithChildrenShortcut);
    escape->setAutoRepeat(false);
    connect(escape, &QShortcut::activated, this, &MainWindow::handleEscape);

    player1Graphics.placeholderColor = Qt::darkRed;
    player2Graphics.placeholderColor = Qt::darkBlue;

    applyHeroes();

    initSoundEffects();
    initMusicControls();
    initStageSelector();
    for (auto *item : menuItems()) item->installEventFilter(this);
    stageMenu->installEventFilter(this);

    auto *timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(CombatTickMs);
    updateControls();
}

void MainWindow::initMusicControls()
{
    musicMuteButton->setCheckable(true);
    musicMuteButton->setChecked(backgroundMusic->isMuted());
    musicMuteButton->setText(backgroundMusic->isMuted() ? QStringLiteral("音乐：关")
                                                      : QStringLiteral("音乐：开"));
    musicMuteButton->setToolTip(QStringLiteral("切换背景音乐静音（M）"));
    connect(musicMuteButton, &QPushButton::toggled, this, [this](bool muted) {
        backgroundMusic->setMuted(muted);
        musicMuteButton->setText(muted ? QStringLiteral("音乐：关") : QStringLiteral("音乐：开"));
    });

    musicVolumeSlider->setRange(0, 100);
    musicVolumeSlider->setPageStep(10);
    musicVolumeSlider->setValue(backgroundMusic->volume());
    musicVolumeSlider->setAccessibleName(QStringLiteral("背景音乐音量"));
    musicVolumeSlider->setToolTip(QStringLiteral("背景音乐音量 %1%").arg(musicVolumeSlider->value()));
    musicVolumeSlider->setStyleSheet(QStringLiteral(
        "QSlider::groove:horizontal { height: 4px; background: #665459; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #edb578; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 14px; margin: -5px 0; background: #f6f0e1;"
        " border: 1px solid #947365; border-radius: 7px; }"
        "QSlider:focus::handle:horizontal { border-color: #a7c7ff; }"));
    connect(musicVolumeSlider, &QSlider::valueChanged, this, [this](int percent) {
        backgroundMusic->setVolume(percent);
        musicVolumeSlider->setToolTip(QStringLiteral("背景音乐音量 %1%").arg(percent));
    });

    auto *muteShortcut = new QShortcut(QKeySequence(Qt::Key_M), this);
    muteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    muteShortcut->setAutoRepeat(false);
    connect(muteShortcut, &QShortcut::activated, musicMuteButton, &QPushButton::toggle);
}

void MainWindow::initStageSelector()
{
    stageSelector->setAccessibleName(QStringLiteral("选择地图"));
    stageSelector->setToolTip(QStringLiteral("地图同时用于对战和木桩练习"));
    stageMenu->setStyleSheet(QStringLiteral(
        "QMenu { color: #f6ebda; background: #29242f; border: 1px solid #70585b; padding: 4px; }"
        "QMenu::item { padding: 8px 16px; }"
        "QMenu::item:selected { background: #6c504b; color: #ffffff; }"));
    for (int index = 0; index < static_cast<int>(StageId::Count); ++index) {
        const auto id = static_cast<StageId>(index);
        const Stage &stage = stageFor(id);
        const QString layout = stage.platforms.isEmpty()
            ? QStringLiteral("平地")
            : QStringLiteral("%1 个浮板").arg(stage.platforms.size());
        const QString label = QStringLiteral("%1 · %2").arg(stage.name, layout);
        auto *action = stageMenu->addAction(label);
        action->setCheckable(true);
        action->setData(index);
        if (id == m_stageId) stageSelector->setText(label + QStringLiteral("  ▾"));
        connect(action, &QAction::triggered, this, [this, id, label] {
            m_stageId = id;
            stageSelector->setText(label + QStringLiteral("  ▾"));
            update();
        });
    }
    connect(stageSelector, &QPushButton::clicked, this, [this] {
        QAction *selected = nullptr;
        for (auto *action : stageMenu->actions()) {
            const bool current = action->data().toInt() == static_cast<int>(m_stageId);
            action->setChecked(current);
            if (current) selected = action;
        }
        stageMenu->popup(stageSelector->mapToGlobal(QPoint(0, stageSelector->height() + 4)));
        stageMenu->setActiveAction(selected);
    });
    connect(stageMenu, &QMenu::aboutToHide, stageSelector, [this] { stageSelector->setFocus(); });
}

void MainWindow::initSoundEffects()
{
    const char *names[] = {"hit", "jump", "ko", "block", "parry"};
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

void MainWindow::clearMovement()
{
    p1Left = p1Right = p2Left = p2Right = false;
    p1JumpHeld = p1FallHeld = p2JumpHeld = p2FallHeld = false;
    m_p1Blink.clearTap();
    m_p2Blink.clearTap();
    m_p1Blink.clearFollowUp();
    m_p2Blink.clearFollowUp();
    player1Logic.clearCounterOpportunity();
    player2Logic.clearCounterOpportunity();
    player1Logic.setGuardHeld(false);
    if (m_mode == MatchMode::Local) player2Logic.setGuardHeld(false);
}

void MainWindow::jump(Fighter &fighter, const char *soundAlias)
{
    if (StagePhysics::jump(fighter)) playSfx("jump", soundAlias);
}

void MainWindow::applyHeroes()
{
    loadHeroAssets(player1Graphics, m_p1Hero);
    loadHeroAssets(player2Graphics, m_p2Hero);
}

void MainWindow::openHeroPanel(HeroSelect::Mode mode)
{
    // 两步选择都画在画布里的覆盖面板上：先挑左边（1P / 玩家），再挑右边（电脑 / 2P）。
    // 任何一步返回都留在菜单，不动当前的英雄配置。
    m_heroPanel = HeroSelect::Panel{};
    m_heroPanel.mode = mode;
    m_heroPanel.step = 0;
    m_heroPanel.hover = -1;
    m_heroPanel.p1 = m_p1Hero;
    m_heroPanel.p2 = m_p2Hero;
    m_heroPanel.setCursorHero(m_p1Hero);

    m_state = GameState::HeroSelect;
    updateControls();
    setFocus();
    update();
}

void MainWindow::startComputerMatch()
{
    openHeroPanel(HeroSelect::Mode::VsComputer);
}

void MainWindow::startLocalMatch()
{
    openHeroPanel(HeroSelect::Mode::VsLocal);
}

void MainWindow::startTrainingMatch()
{
    // 木桩练习只要选自己用的角色，面板只有一步
    openHeroPanel(HeroSelect::Mode::Training);
}

void MainWindow::confirmHeroSelection()
{
    const HeroKind picked = m_heroPanel.cursorHero();
    if (m_heroPanel.singleStep()) {
        // 木桩练习：只写回玩家角色，右边是人桩，没有对手英雄可选
        m_heroPanel.p1 = picked;
        m_p1Hero = picked;
        applyHeroes();
        startMatch(MatchMode::Training);
        setFocus();
        return;
    }
    if (!m_heroPanel.opponentStep()) {
        m_heroPanel.p1 = picked;
        m_heroPanel.step = 1;
        // 后选的一方默认换成另一个角色，避免一进去就是镜像局（仍可手动改成同一个）
        m_heroPanel.setCursorHero(picked == HeroKind::HeroKnight ? HeroKind::MartialHero
                                                                 : HeroKind::HeroKnight);
        m_heroPanel.p2 = m_heroPanel.cursorHero();
        m_heroPanel.hover = -1;
        update();
        return;
    }

    m_heroPanel.p2 = picked;
    m_p1Hero = m_heroPanel.p1;
    m_p2Hero = m_heroPanel.p2;
    applyHeroes();
    const bool local = m_heroPanel.mode == HeroSelect::Mode::VsLocal;
    if (!local) computerPlayer.setHero(m_p2Hero);
    startMatch(local ? MatchMode::Local : MatchMode::Computer);
    setFocus();
}

void MainWindow::stepBackHeroSelection()
{
    if (m_state != GameState::HeroSelect) return;
    if (!m_heroPanel.opponentStep()) {
        returnToMenu();
        return;
    }
    m_heroPanel.step = 0;
    m_heroPanel.setCursorHero(m_heroPanel.p1);
    m_heroPanel.hover = -1;
    update();
}

void MainWindow::handleEscape()
{
    if (m_state == GameState::HeroSelect) {
        stepBackHeroSelection();
        return;
    }
    returnToMenu();
}

void MainWindow::startMatch(MatchMode mode)
{
    m_mode = mode;
    m_dummyMode = DummyMode::Idle;
    startRound();
}

void MainWindow::startRound()
{
    const bool training = m_mode == MatchMode::Training;
    // 起始高度必须跟着角色的帧尺寸走：200 帧的角色 y = GroundY-200 才刚好站在地面上
    const int p1Frame = heroTraits(m_p1Hero).frameSize;
    const int p2Frame = heroTraits(m_p2Hero).frameSize;
    if (training) {
        trainingDummy.reset(505, GameUI::GroundY + 1 - trainingDummy.getBodyBox().height());
        trainingDummy.onGround = true;
    } else {
        player2Graphics.reset(600, GameUI::GroundY - p2Frame);
        player2Graphics.setFacing(false);
    }

    Fighter &opponent = opponentFighter();
    player1Graphics.reset(training ? 210 : 100, GameUI::GroundY - p1Frame);
    player1Graphics.setFacing(true);
    player1Logic = PlayerLogic(&player1Graphics, heroTraits(m_p1Hero).skill);
    player2Logic = PlayerLogic(&opponent, training ? SkillKind::None : heroTraits(m_p2Hero).skill);
    player2Logic.setGuardHeld(training && m_dummyMode == DummyMode::Guard);
    m_p1Blink = {};
    m_p2Blink = {};
    m_p1Skill = {};
    m_p2Skill = {};
    if (m_mode == MatchMode::Computer) {
        computerPlayer.setHero(m_p2Hero);
        computerPlayer.reset();
    }
    clearMovement();

    m_p1HealthTrail.reset(player1Graphics.hp);
    m_p2HealthTrail.reset(opponent.hp);
    m_lastDamage = m_totalDamage = 0;
    m_dummyRefillAt = 0;
    m_dummyAttackDelay = DummyInitialDelay;
    m_trainingPlayerRefillAt = 0;
    m_hitboxFlashTicks = 0;
    m_p1HitSpark = {};
    m_p2HitSpark = {};
    m_hitStopTicks = 0;
    m_shakeTicks = 0;
    m_shakeAmplitude = 0;
    m_countdownLeft = COUNTDOWN_SECONDS;
    m_remainingMs = ROUND_SECONDS * 1000;
    m_roundClock.start();
    m_state = training ? GameState::Playing : GameState::Countdown;
    if (training) backgroundMusic->playMenu();
    else backgroundMusic->playStage(m_stageId);
    updateControls();
}

void MainWindow::returnToMenu()
{
    if (m_state == GameState::Menu) {
        if (m_adjustingVolume) {
            m_adjustingVolume = false;
            update();
        }
        return;
    }

    clearMovement();
    m_state = GameState::Menu;
    // 回菜单必须清掉顿帧/震动，否则会卡在边缘抖动
    m_hitStopTicks = 0;
    m_shakeTicks = 0;
    m_shakeAmplitude = 0;
    backgroundMusic->playMenu();
    updateControls();
}

void MainWindow::checkWin()
{
    const int p1Hp = player1Graphics.hp;
    const int p2Hp = player2Graphics.hp;
    const bool knockout = p1Hp <= 0 || p2Hp <= 0;
    if (!knockout && m_remainingMs > 0) return;

    if (p1Hp > p2Hp)
        m_winner = GameUI::Winner::PlayerOne;
    else if (p2Hp > p1Hp)
        m_winner = GameUI::Winner::PlayerTwo;
    else
        m_winner = GameUI::Winner::Draw;

    if (knockout) {
        m_resultDetail = m_winner == GameUI::Winner::Draw ? QStringLiteral("双方同时倒下")
                                                        : QStringLiteral("击倒对手");
    } else {
        m_resultDetail = QStringLiteral("时间到 · 玩家 1 剩余 %1 · %2 剩余 %3")
                             .arg(p1Hp).arg(opponentName()).arg(p2Hp);
    }

    clearMovement();
    m_state = GameState::GameOver;
    playSfx("ko", "koalias");
    updateControls();
}

void MainWindow::updateControls()
{
    const bool inMenu = m_state == GameState::Menu;
    localButton->setVisible(inMenu);
    computerButton->setVisible(inMenu);
    trainingButton->setVisible(inMenu);
    keySettingsButton->setVisible(inMenu);
    musicMuteButton->setVisible(inMenu);
    musicVolumeSlider->setVisible(inMenu);
    stageSelector->setVisible(inMenu);
    retryButton->setVisible(m_state == GameState::GameOver);
    menuButton->setVisible(m_state == GameState::GameOver);
    m_adjustingVolume = false;
    // 选择面板里鼠标是手型光标，离开面板要还原
    if (m_state != GameState::HeroSelect) unsetCursor();
    if (inMenu) localButton->setFocus();
    else setFocus();
    update();
}

Fighter &MainWindow::opponentFighter()
{
    return m_mode == MatchMode::Training ? trainingDummy : player2Graphics;
}

QString MainWindow::opponentName() const
{
    return m_mode == MatchMode::Computer ? QStringLiteral("电脑") : QStringLiteral("玩家 2");
}

QRectF MainWindow::canvasRect() const
{
    const qreal scale = qMin(qreal(width()) / GameUI::Width, qreal(height()) / GameUI::Height);
    QRectF area(0, 0, GameUI::Width * scale, GameUI::Height * scale);
    area.moveCenter(QPointF(width() / 2.0, height() / 2.0));
    return area;
}

// 把窗口坐标换算回画布坐标（paintEvent 里的 translate + scale 的逆运算）
QPointF MainWindow::gamePoint(const QPointF &widgetPos) const
{
    const QRectF area = canvasRect();
    const qreal scale = area.width() / GameUI::Width;
    if (scale <= 0) return widgetPos;
    return QPointF((widgetPos.x() - area.left()) / scale, (widgetPos.y() - area.top()) / scale);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (m_state == GameState::HeroSelect && event->button() == Qt::LeftButton) {
        const int hit = HeroSelect::hitTest(gamePoint(event->position()));
        if (hit == 0 || hit == 1) {
            if (m_heroPanel.cursor == hit) confirmHeroSelection();  // 二次点击 = 确认
            else {
                m_heroPanel.cursor = hit;
                m_heroPanel.hover = hit;
                update();
            }
        } else if (hit == 2) {
            stepBackHeroSelection();
        } else if (hit == 3) {
            confirmHeroSelection();
        }
        return;
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 双击卡片 = 选中并直接确认，和原来弹窗里的习惯保持一致
    if (m_state == GameState::HeroSelect && event->button() == Qt::LeftButton) {
        const int hit = HeroSelect::hitTest(gamePoint(event->position()));
        if (hit == 0 || hit == 1) {
            m_heroPanel.cursor = hit;
            confirmHeroSelection();
        }
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_state == GameState::HeroSelect) {
        const int hit = HeroSelect::hitTest(gamePoint(event->position()));
        if (hit != m_heroPanel.hover) {
            m_heroPanel.hover = hit;
            update();
        }
        setCursor(hit >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const QRectF area = canvasRect();
    const qreal scale = area.width() / GameUI::Width;
    QFont buttonFont = font();
    buttonFont.setPixelSize(qRound(18 * scale));
    buttonFont.setBold(true);

    const auto scaledBounds = [&](const QRect &bounds) {
        return QRectF(area.x() + bounds.x() * scale, area.y() + bounds.y() * scale,
                      bounds.width() * scale, bounds.height() * scale).toAlignedRect();
    };
    const auto placeButton = [&](QPushButton *button, const QRect &bounds) {
        button->setGeometry(scaledBounds(bounds));
        button->setFont(buttonFont);
    };

    placeButton(localButton, GameUI::MenuPrimaryButton);
    placeButton(computerButton, GameUI::MenuSecondaryButton);
    placeButton(trainingButton, GameUI::MenuTrainingButton);
    placeButton(keySettingsButton, GameUI::MenuKeySettingsButton);
    placeButton(retryButton, GameUI::ResultPrimaryButton);
    placeButton(menuButton, GameUI::ResultSecondaryButton);
    placeButton(musicMuteButton, GameUI::MusicMuteButton);
    QFont musicFont = buttonFont;
    musicFont.setPixelSize(qRound(14 * scale));
    musicMuteButton->setFont(musicFont);
    stageSelector->setGeometry(scaledBounds(GameUI::StageSelector));
    stageSelector->setFont(musicFont);
    musicVolumeSlider->setGeometry(scaledBounds(GameUI::MusicVolumeSlider));
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate) clearMovement();
    return QWidget::event(event);
}

void MainWindow::gameLoop()
{
    if (m_state == GameState::Countdown) {
        const int elapsedSeconds = static_cast<int>(m_roundClock.elapsed() / 1000);
        m_countdownLeft = COUNTDOWN_SECONDS - elapsedSeconds;
        if (m_countdownLeft <= 0) {
            m_roundClock.restart();
            m_state = GameState::Playing;
        }
        update();
        return;
    }

    // 菜单与英雄选择面板都没有战斗逻辑要推进
    if (m_state == GameState::Menu || m_state == GameState::HeroSelect) return;

    Fighter &opponent = opponentFighter();
    const bool hasHitFlash = player1Graphics.flashTicks > 0 || opponent.flashTicks > 0
                             || player1Graphics.blockFlashTicks > 0 || opponent.blockFlashTicks > 0
                             || m_hitboxFlashTicks > 0
                             || m_p1HitSpark.ticks > 0 || m_p2HitSpark.ticks > 0
                             || m_p1Blink.trailTicks() > 0 || m_p2Blink.trailTicks() > 0
                             || player1Logic.perfectFlashTicks() > 0 || player2Logic.perfectFlashTicks() > 0;
    m_p1Blink.advance();
    m_p2Blink.advance();
    if (player1Graphics.flashTicks > 0) --player1Graphics.flashTicks;
    if (opponent.flashTicks > 0) --opponent.flashTicks;
    if (player1Graphics.blockFlashTicks > 0) --player1Graphics.blockFlashTicks;
    if (opponent.blockFlashTicks > 0) --opponent.blockFlashTicks;
    if (m_hitboxFlashTicks > 0) --m_hitboxFlashTicks;
    if (m_p1HitSpark.ticks > 0) --m_p1HitSpark.ticks;
    if (m_p2HitSpark.ticks > 0) --m_p2HitSpark.ticks;
    // 震动计时独立递减：时长可能大于顿帧，必须自己走完，否则会永久抖动
    if (m_shakeTicks > 0) {
        --m_shakeTicks;
        if (m_shakeTicks == 0) m_shakeAmplitude = 0;
    }

    // 顿帧：命中瞬间把整个战场冻住几帧。物理/逻辑/动画全部暂停，只重绘
    if (m_hitStopTicks > 0) {
        --m_hitStopTicks;
        update();
        return;
    }

    if (m_state == GameState::GameOver) {
        const auto finishAnimation = [](Fighter &fighter, PlayerLogic &logic) {
            if (!logic.isPlayingAttack() && !logic.isCasting() && logic.perfectFlashTicks() == 0)
                return false;
            logic.update();
            fighter.updateAnimation();
            return true;
        };
        const bool p1Animating = finishAnimation(player1Graphics, player1Logic);
        const bool p2Animating = finishAnimation(opponent, player2Logic);
        const bool effectsVisible = m_p1Skill.lifetime > 0 || m_p2Skill.lifetime > 0;
        m_p1Skill.advance(0, GameUI::Width - 1);
        m_p2Skill.advance(0, GameUI::Width - 1);
        if (hasHitFlash || p1Animating || p2Animating || effectsVisible) update();
        return;
    }

    const bool training = m_mode == MatchMode::Training;
    const auto moveHuman = [](Fighter &fighter, const PlayerLogic &logic, bool left, bool right) {
        // 击退 / 破防硬直期间由逻辑接管速度，按键抢不回控制权
        if (logic.overrideMovement(fighter)) return;
        if (logic.isLunging()) {
            fighter.vx = logic.lungeVelocity();
            return;
        }
        // 举盾：锁走动，但方向键可以原地转身。
        // 不做"自动面向对手"——转身是玩家自己的判断和操作成本，盾强在正面而不是无死角。
        if (logic.isGuarding()) {
            const int guardDir = static_cast<int>(right) - static_cast<int>(left);
            if (fighter.hp > 0 && guardDir != 0) fighter.setFacing(guardDir > 0);
            fighter.vx = 0;
            return;
        }
        const bool locked = fighter.hp <= 0 || logic.isCasting() || logic.isCountering();
        const int direction = static_cast<int>(right) - static_cast<int>(left);
        if (!locked && direction != 0) fighter.setFacing(direction > 0);
        fighter.vx = locked ? 0 : direction * MOVE_SPD;
    };
    moveHuman(player1Graphics, player1Logic, p1Left, p1Right);

    if (m_mode == MatchMode::Computer) {
        const auto move = computerPlayer.update(opponent, player1Graphics, player2Logic);
        if (!player2Logic.overrideMovement(opponent))   // 电脑也会被打飞
            opponent.vx = move.direction * COMPUTER_MOVE_SPD;
        if (move.jump) jump(opponent, "jumpalias2");
        if (move.fastFall) StagePhysics::fastFall(opponent);
    } else if (training) {
        // 木桩不接击退：练连招时它必须站桩，否则打几下就被推出攻击范围
        opponent.vx = 0;
        player2Logic.clearKnockback();
        if (m_dummyMode != DummyMode::Idle && !player2Logic.isCommittedToAttack())
            opponent.setFacing(player1Graphics.getBodyBox().center().x() > opponent.getBodyBox().center().x());
        if (m_dummyMode == DummyMode::Attack && opponent.hp > 0) {
            if (m_dummyAttackDelay > 0) --m_dummyAttackDelay;
            if (m_dummyAttackDelay == 0 && player2Logic.attack())
                m_dummyAttackDelay = DummyAttackInterval;
        }
    } else {
        moveHuman(opponent, player2Logic, p2Left, p2Right);
    }

    const Stage &stage = stageFor(m_stageId);
    StagePhysics::step(player1Graphics, stage);
    if (!training) StagePhysics::step(opponent, stage);
    const bool releaseP1 = player1Logic.update();
    const bool releaseP2 = player2Logic.update();

    const auto showHit = [&](const QRect &attack, const QRect &contact,
                             const HitResult &result, bool fromP1) {
        if (result.damage > 0 || result.blocked) {
            m_lastHitBox = attack;
            m_hitboxFlashTicks = 10;

            // 顿帧 + 震屏：按结果分配"重量"。格挡最轻，破防最重
            int stopTicks = 4, shakeAmp = 3;
            if (result.guardBroke)      { stopTicks = 8; shakeAmp = 5; }
            else if (result.perfect)    { stopTicks = 3; shakeAmp = 2; }
            else if (result.blocked)    { stopTicks = 2; shakeAmp = 1; }
            m_hitStopTicks = qMax(m_hitStopTicks, stopTicks);
            m_shakeTicks = qMax(m_shakeTicks, ShakeTicks);
            m_shakeAmplitude = qMax(m_shakeAmplitude, shakeAmp);

            if (result.perfect) {
                playSfx("parry", "parryalias");
            } else if (result.blocked) {
                playSfx("block", "blockalias");
            } else {
                (fromP1 ? m_p2Blink : m_p1Blink).clearFollowUp();
                HitSpark &spark = fromP1 ? m_p2HitSpark : m_p1HitSpark;
                spark = {contact.center(), HitSparkTicks};
                playSfx("hit", fromP1 ? "hitalias" : "hitalias2");
            }
        }
        return result.damage;
    };
    const auto resolveNormal = [&](const Fighter &target, PlayerLogic &attackerLogic,
                                   PlayerLogic &targetLogic, bool fromP1) {
        const QRect attack = attackerLogic.activeAttackBox();
        const QRect contact = attack.intersected(target.getBodyBox());
        if (contact.isEmpty()) return 0;
        return showHit(attack, contact, attackerLogic.resolveHit(targetLogic), fromP1);
    };
    int damage = resolveNormal(opponent, player1Logic, player2Logic, true);
    if (!training || m_dummyMode == DummyMode::Attack)
        resolveNormal(player1Graphics, player2Logic, player1Logic, false);

    m_p1Skill.advance(0, GameUI::Width - 1);
    m_p2Skill.advance(0, GameUI::Width - 1);
    if (releaseP1 && player1Logic.isCasting())
        m_p1Skill = SkillEffect::spawn(player1Logic.skill(), player1Graphics);
    if (releaseP2 && player2Logic.isCasting())
        m_p2Skill = SkillEffect::spawn(player2Logic.skill(), opponent);

    const auto resolveSkill = [&](SkillEffect &effect, const Fighter &target,
                                  PlayerLogic &logic, bool fromP1) {
        const QRect attack = effect.hitBox();
        const QRect body = target.getBodyBox();
        const QRect contact = attack.intersected(body);
        if (target.hp <= 0 || contact.isEmpty()) return 0;

        const int sourceX = effect.kind == SkillKind::SwordWave ? body.center().x() - effect.direction
                                                               : attack.center().x();
        const HitResult result = logic.receiveHit(skillSpec(effect.kind).damage,
                                                  skillSpec(effect.kind).guardDamage,
                                                  sourceX,
                                                  skillSpec(effect.kind).knockback);
        effect.markHit();
        const int dealt = showHit(attack, contact, result, fromP1);
        // 技能是"大招"，命中反馈再加一档
        if (result.damage > 0 && !result.blocked) {
            m_hitStopTicks = qMax(m_hitStopTicks, 7);
            m_shakeTicks = qMax(m_shakeTicks, ShakeTicks);
            m_shakeAmplitude = qMax(m_shakeAmplitude, 4);
        }
        return dealt;
    };
    damage += resolveSkill(m_p1Skill, opponent, player2Logic, true);
    if (!training) resolveSkill(m_p2Skill, player1Graphics, player1Logic, false);

    player1Graphics.updateAnimation();
    opponent.updateAnimation();

    const qint64 elapsed = m_roundClock.elapsed();
    if (training) {
        if (damage > 0) {
            m_lastDamage = damage;
            m_totalDamage += damage;
            if (opponent.hp == 0) m_dummyRefillAt = elapsed + 650;
        }
        if (m_dummyRefillAt > 0 && elapsed >= m_dummyRefillAt) {
            opponent.hp = opponent.maxHp;
            player2Logic = PlayerLogic(&opponent);
            player2Logic.setGuardHeld(m_dummyMode == DummyMode::Guard);
            m_dummyRefillAt = 0;
            m_dummyAttackDelay = DummyInitialDelay;
        }
        if (player1Graphics.hp == 0 && m_trainingPlayerRefillAt == 0)
            m_trainingPlayerRefillAt = elapsed + 650;
        if (m_trainingPlayerRefillAt > 0 && elapsed >= m_trainingPlayerRefillAt) {
            player1Graphics.hp = player1Graphics.maxHp;
            m_trainingPlayerRefillAt = 0;
        }
    }

    m_p1HealthTrail.update(player1Graphics.hp, elapsed);
    m_p2HealthTrail.update(opponent.hp, elapsed);
    if (!training) {
        m_remainingMs = static_cast<int>(qMax<qint64>(0, ROUND_SECONDS * 1000 - elapsed));
        checkWin();
    }
    update();
}

void MainWindow::drawScene(QPainter &p)
{
    const bool training = m_mode == MatchMode::Training;
    const Fighter &opponent = opponentFighter();
    const Stage &stage = stageFor(m_stageId);
    GameUI::drawArena(p, stage);
    auto drawShadow = [&](const Fighter &f) {
        QRect b = f.getBodyBox();
        const int surface = StagePhysics::surfaceBelow(b, stage);
        int air = surface - b.bottom();
        if (air < 0) air = 0;
        double scale = 1.0 - qMin(air, 120) / 200.0;
        if (scale < 0.25) scale = 0.25;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, (int)(100 * scale)));
        p.drawEllipse(QPointF(b.center().x(), surface + 5),
                      static_cast<double>(b.width()) / 3 * scale, 10.0 * scale);
    };
    drawShadow(player1Graphics);
    drawShadow(opponent);

    GameUI::drawBlinkTrail(p, m_p1Blink, true);
    GameUI::drawBlinkTrail(p, m_p2Blink, false);
    player1Graphics.draw(p);
    if (training) {
        GameUI::drawTrainingDummy(p, opponent.getBodyBox(), opponent.hp,
                                  qreal(opponent.flashTicks) / Fighter::HitFlashTicks,
                                  opponent.facingRight, player2Logic.isCommittedToAttack(),
                                  player2Logic.activeAttackBox());
    } else {
        opponent.draw(p);
    }

    if (player1Logic.isLunging())
        GameUI::drawLungeEffect(p, player1Graphics, player1Logic.skill(), player1Logic.lungeTick());
    if (player2Logic.isLunging())
        GameUI::drawLungeEffect(p, opponent, player2Logic.skill(), player2Logic.lungeTick());
    GameUI::drawSkillEffect(p, m_p1Skill);
    GameUI::drawSkillEffect(p, m_p2Skill);
    GameUI::drawGuardEffect(p, player1Graphics, player1Logic.isGuarding());
    GameUI::drawGuardEffect(p, opponent, player2Logic.isGuarding());
    GameUI::drawCounterEffects(p, player1Graphics, player1Logic);
    GameUI::drawCounterEffects(p, opponent, player2Logic);
    for (const HitSpark &spark : {m_p1HitSpark, m_p2HitSpark}) {
        if (spark.ticks > 0)
            GameUI::drawHitSpark(p, spark.position, 1.0 - qreal(spark.ticks) / HitSparkTicks);
    }

    // 头顶身份标签：打电脑只标玩家自己（YOU），本地双人标 1P / 2P
    if (m_mode == MatchMode::Computer) {
        GameUI::drawPlayerTag(p, player1Graphics.getBodyBox(), QStringLiteral("YOU"),
                              QColor("#ff5f52"));
    } else if (m_mode == MatchMode::Local) {
        GameUI::drawPlayerTag(p, player1Graphics.getBodyBox(), QStringLiteral("1P"),
                              QColor("#ff5f52"));
        GameUI::drawPlayerTag(p, opponent.getBodyBox(), QStringLiteral("2P"),
                              QColor("#5aa8ff"));
    }

    const bool finished = m_state == GameState::GameOver;
    const qint64 elapsed = m_roundClock.elapsed();
    const GameUI::HealthBar p1{player1Graphics.hp, player1Graphics.maxHp,
                               finished ? player1Graphics.hp : m_p1HealthTrail.value(elapsed),
                               player1Logic.guardValue(), player1Logic.guardMax(),
                               player1Logic.isGuardBroken(), player1Logic.guardBrokenTicks()};
    const GameUI::HealthBar p2{opponent.hp, opponent.maxHp,
                               finished ? opponent.hp : m_p2HealthTrail.value(elapsed),
                               player2Logic.guardValue(), player2Logic.guardMax(),
                               player2Logic.isGuardBroken(), player2Logic.guardBrokenTicks()};
    if (training) {
        const QString name = m_dummyMode == DummyMode::Attack ? QStringLiteral("出招木桩")
                           : m_dummyMode == DummyMode::Guard ? QStringLiteral("防御木桩")
                           : QStringLiteral("练习木桩");
        GameUI::drawTrainingHud(p, p1, p2, m_lastDamage, m_totalDamage, name);
    } else {
        GameUI::drawHud(p, p1, p2, m_remainingMs, !finished, opponentName());
    }

    GameUI::drawSkillStatus(p, player1Logic.skill(), player1Logic.skillCooldown(),
                           player2Logic.skill(), player2Logic.skillCooldown());
    GameUI::drawBlinkStatus(p, m_p1Blink, m_p2Blink, m_mode == MatchMode::Local);
    GameUI::drawCounterStatus(p, player1Logic, player2Logic, !training);

    if (m_showHitboxes) {
        GameUI::drawSkillHitboxes(p, m_p1Skill.hitBox(), m_p2Skill.hitBox());
        GameUI::drawHitboxes(p, player1Graphics.hp > 0 ? player1Graphics.getBodyBox() : QRect(),
                             opponent.hp > 0 ? opponent.getBodyBox() : QRect(),
                             player1Logic.activeAttackBox(), player2Logic.activeAttackBox(),
                             m_hitboxFlashTicks > 0 ? m_lastHitBox : QRect());
    }

    if (finished)
        GameUI::drawResult(p, m_winner, m_resultDetail, opponentName());
    else if (m_state == GameState::Countdown)
        GameUI::drawRoundStart(p, m_countdownLeft);
    else if (!training && elapsed < START_BANNER_MS)
        GameUI::drawRoundStart(p, 0);

    // 破防瞬间白闪：只覆盖战场区域，HUD 始终保持清晰可读
    const int breakFlash = qMax(player1Logic.guardBreakFlash(), player2Logic.guardBreakFlash());
    if (breakFlash > 0) {
        const qreal t = qreal(breakFlash) / PlayerLogic::GuardBreakFlashTicks;
        QColor flash(255, 236, 226);
        flash.setAlpha(qRound(105.0 * qMin(1.0, t * 2.2)));   // 前段保持、后段快收
        p.fillRect(QRect(0, 112, GameUI::Width, GameUI::Height - 112), flash);
    }
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#0d1019"));
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF area = canvasRect();
    painter.translate(area.topLeft());
    const qreal scale = area.width() / GameUI::Width;
    painter.scale(scale, scale);
    // 屏幕震动：顿帧期间整体抖动，幅度随剩余帧数线性衰减
    if (m_shakeTicks > 0 && m_shakeAmplitude > 0) {
        const int amp = qMax(1, m_shakeAmplitude * m_shakeTicks / ShakeTicks);
        painter.translate(QRandomGenerator::global()->bounded(-amp, amp + 1),
                          QRandomGenerator::global()->bounded(-amp, amp + 1));
    }
    painter.setClipRect(QRect(0, 0, GameUI::Width, GameUI::Height));

    if (m_state == GameState::Menu || m_state == GameState::HeroSelect) {
        // 选择面板浮在菜单画面上：背景仍按当前已选英雄绘制，面板自己压暗
        const bool picking = m_state == GameState::HeroSelect;
        GameUI::drawMenu(painter, player1Graphics, player2Graphics, stageFor(m_stageId),
                         m_adjustingVolume,
                         heroName(picking ? m_heroPanel.p1 : m_p1Hero),
                         heroName(picking ? m_heroPanel.p2 : m_p2Hero));
        if (picking) HeroSelect::draw(painter, m_heroPanel);
    } else {
        drawScene(painter);
    }
}

std::array<QWidget *, 7> MainWindow::menuItems() const
{
    return {localButton, computerButton, trainingButton, keySettingsButton,
            stageSelector, musicMuteButton, musicVolumeSlider};
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_state != GameState::Menu) return QWidget::eventFilter(watched, event);
    if (watched == musicVolumeSlider && event->type() == QEvent::FocusOut && m_adjustingVolume)
        returnToMenu();

    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (watched == stageMenu) {
            if (key->key() == Qt::Key_Space) {
                if (!key->isAutoRepeat()) {
                    auto *selected = stageMenu->activeAction();
                    if (selected && selected->isEnabled()) {
                        stageMenu->hide();
                        selected->trigger();
                    }
                }
                return true;
            }        } else if (handleMenuKey(key)) {
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool MainWindow::handleMenuKey(QKeyEvent *event)
{
    int direction = 0;
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Left: direction = -1; break;
    case Qt::Key_Down:
    case Qt::Key_Right: direction = 1; break;
    case Qt::Key_Space: break;
    case Qt::Key_Escape:
        if (!m_adjustingVolume) return false;
        returnToMenu();
        return true;
    default: return false;
    }

    const auto items = menuItems();
    const auto focused = std::find(items.cbegin(), items.cend(), focusWidget());
    const int current = focused == items.cend() ? 0 : static_cast<int>(focused - items.cbegin());
    const int count = static_cast<int>(items.size());
    QWidget *selected = items[current];

    if (direction != 0) {
        if (m_adjustingVolume) {
            musicVolumeSlider->setValue(musicVolumeSlider->value() + 5 * direction);
        } else {
            items[(current + direction + count) % count]->setFocus();
        }
    } else if (!event->isAutoRepeat()) {
        if (auto *button = qobject_cast<QPushButton *>(selected)) {
            button->click();
        } else {
            m_adjustingVolume = !m_adjustingVolume;
            musicVolumeSlider->setFocus();
            update();
        }
    }
    return true;
}

void MainWindow::openKeySettings()
{
    KeySettingsDialog dialog(m_bindings, this);
    dialog.exec();
    keySettingsButton->setFocus();
}

void MainWindow::applyControl(int player, ControlAction action, bool pressed)
{
    const bool first = player == 0;
    Fighter &fighter = first ? player1Graphics : player2Graphics;
    PlayerLogic &logic = first ? player1Logic : player2Logic;
    Blink &blink = first ? m_p1Blink : m_p2Blink;
    switch (action) {
    case ControlAction::Left:
    case ControlAction::Right: {
        const bool right = action == ControlAction::Right;
        bool &leftHeld = first ? p1Left : p2Left;
        bool &rightHeld = first ? p1Right : p2Right;
        bool &held = right ? rightHeld : leftHeld;
        if (held == pressed) break;

        held = pressed;
        if (pressed) {
            const bool singleDirection = !(leftHeld && rightHeld);
            if (blink.tap(right ? 1 : -1, m_roundClock.elapsed(), singleDirection && logic.canBlink(),
                          fighter, stageFor(m_stageId))) {
                update();
            }
        }
        break;
    }
    case ControlAction::Guard: logic.setGuardHeld(pressed); break;
    case ControlAction::Jump:
        (first ? p1JumpHeld : p2JumpHeld) = pressed;
        if (pressed && !logic.isGuarding() && !logic.isCasting() && !logic.isLunging() && !logic.isCountering())
            jump(fighter, first ? "jumpalias" : "jumpalias2");
        break;
    case ControlAction::FastFall:
        (first ? p1FallHeld : p2FallHeld) = pressed;
        if (pressed && !logic.isGuarding() && !logic.isCasting() && !logic.isLunging() && !logic.isCountering())
            StagePhysics::fastFall(fighter);
        break;
    case ControlAction::Attack:
        if (!pressed) break;
        // 组合技：按住跳跃键 -> 跃击；按住快速下落键 -> 下劈
        if ((first ? p1JumpHeld : p2JumpHeld) && logic.airSlash()) { update(); break; }
        if ((first ? p1FallHeld : p2FallHeld) && logic.downSlash()) { update(); break; }
        if (logic.counterReadyTicks() > 0) {
            if (logic.counter()) {
                blink.clearFollowUp();
                update();
            }
        } else if (blink.followUpTicks() > 0) {
            if (logic.lunge(blink.followUpDirection())) {
                blink.clearFollowUp();
                update();
            }
        } else {
            logic.attack();
        }
        break;
    case ControlAction::Skill:
        if (pressed) logic.useSkill();
        break;
    default: break;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (m_state == GameState::HeroSelect) {
        if (e->isAutoRepeat()) return;
        switch (e->key()) {
        case Qt::Key_Left:
        case Qt::Key_A:
        case Qt::Key_Tab:
            m_heroPanel.cursor = 0;
            m_heroPanel.hover = -1;
            update();
            return;
        case Qt::Key_Right:
        case Qt::Key_D:
            m_heroPanel.cursor = 1;
            m_heroPanel.hover = -1;
            update();
            return;
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            confirmHeroSelection();
            return;
        case Qt::Key_Backspace:
        case Qt::Key_Back:
            stepBackHeroSelection();
            return;
        default:
            return;
        }
    }
    if (m_state == GameState::Menu && handleMenuKey(e)) return;
    if (e->isAutoRepeat()) return;
    if (e->key() == Qt::Key_Escape) {
        returnToMenu();
        return;
    }
    if (e->key() == Qt::Key_F12 && m_state != GameState::Menu) {
        m_showHitboxes = !m_showHitboxes;
        update();
        return;
    }
    if (m_state == GameState::Countdown) return;
    if (m_state != GameState::Playing) {
        if (m_state == GameState::GameOver && e->key() == Qt::Key_Space) startRound();
        else QWidget::keyPressEvent(e);
        return;
    }

    if (m_mode == MatchMode::Training) {
        if (e->key() == Qt::Key_T) {
            switch (m_dummyMode) {
            case DummyMode::Idle: m_dummyMode = DummyMode::Guard; break;
            case DummyMode::Guard: m_dummyMode = DummyMode::Attack; break;
            case DummyMode::Attack: m_dummyMode = DummyMode::Idle; break;
            }
            trainingDummy.setState(AnimState::Idle);
            player2Logic = PlayerLogic(&trainingDummy);
            player2Logic.setGuardHeld(m_dummyMode == DummyMode::Guard);
            m_dummyAttackDelay = DummyInitialDelay;
            return;
        }
        if (e->key() == Qt::Key_R) {
            startRound();
            return;
        }
    }

    applyControl(0, m_bindings.actionForKey(0, e->key()), true);
    if (m_mode == MatchMode::Local) applyControl(1, m_bindings.actionForKey(1, e->key()), true);
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat() || m_state != GameState::Playing) return;
    applyControl(0, m_bindings.actionForKey(0, e->key()), false);
    if (m_mode == MatchMode::Local) applyControl(1, m_bindings.actionForKey(1, e->key()), false);
}
