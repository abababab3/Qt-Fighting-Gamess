#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QElapsedTimer>
#include <QWidget>

#include "blink.h"
#include "computerplayer.h"
#include "fighter.h"
#include "gameui.h"
#include "hero.h"
#include "heroselectpanel.h"
#include "keybindings.h"
#include "playerlogic.h"

class BackgroundMusic;
class QMenu;
class QPushButton;
class QSlider;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // HeroSelect：菜单与开局之间的覆盖面板，画在游戏画布里而不是独立窗口
    enum class GameState { Menu, HeroSelect, Countdown, Playing, GameOver };

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class MatchMode { Local, Computer, Training };
    enum class DummyMode { Idle, Guard, Attack };

    struct HitSpark {
        QPoint position;
        int ticks = 0;
    };

    void gameLoop();
    void clearMovement();
    void jump(Fighter &fighter, const char *soundAlias);
    void startMatch(MatchMode mode);
    void openHeroPanel(HeroSelect::Mode mode);
    void startComputerMatch();
    void startLocalMatch();
    void startTrainingMatch();
    void confirmHeroSelection();
    void stepBackHeroSelection();
    void handleEscape();
    void applyHeroes();
    void startRound();
    void returnToMenu();
    void checkWin();
    void updateControls();
    Fighter &opponentFighter();
    QString opponentName() const;
    QRectF canvasRect() const;
    QPointF gamePoint(const QPointF &widgetPos) const;
    void initSoundEffects();
    void initMusicControls();
    void initStageSelector();
    std::array<QWidget *, 7> menuItems() const;
    bool handleMenuKey(QKeyEvent *event);
    void openKeySettings();
    void applyControl(int player, ControlAction action, bool pressed);
    void playSfx(const char *resName, const char *alias);
    void drawScene(QPainter &p);

    QPushButton *localButton;
    QPushButton *computerButton;
    QPushButton *trainingButton;
    QPushButton *keySettingsButton;
    QPushButton *retryButton;
    QPushButton *menuButton;
    QPushButton *musicMuteButton;
    QSlider *musicVolumeSlider;
    QPushButton *stageSelector;
    QMenu *stageMenu;
    BackgroundMusic *backgroundMusic;
    KeyBindings m_bindings;
    bool m_adjustingVolume = false;

    Fighter player1Graphics;
    Fighter player2Graphics;
    Fighter trainingDummy{QSize(46, 56)};
    PlayerLogic player1Logic;
    PlayerLogic player2Logic;
    ComputerPlayer computerPlayer;

    GameState m_state = GameState::Menu;
    GameUI::Winner m_winner = GameUI::Winner::Draw;
    QString m_resultDetail;
    MatchMode m_mode = MatchMode::Local;
    // 玩家 1 与对手（电脑 / 玩家 2）各自选的角色。默认沿用改造前的组合：
    // 左边 Martial Hero，右边 Hero Knight。
    HeroKind m_p1Hero = HeroKind::MartialHero;
    HeroKind m_p2Hero = HeroKind::HeroKnight;
    // 覆盖式选择面板的临时状态（确认后才写回 m_p1Hero / m_p2Hero）
    HeroSelect::Panel m_heroPanel;
    StageId m_stageId = StageId::Moonlit;
    QElapsedTimer m_roundClock;
    GameUI::HealthTrail m_p1HealthTrail;
    GameUI::HealthTrail m_p2HealthTrail;
    int m_countdownLeft = 0;
    int m_remainingMs = 0;

    Blink m_p1Blink;
    Blink m_p2Blink;
    SkillEffect m_p1Skill;
    SkillEffect m_p2Skill;
    DummyMode m_dummyMode = DummyMode::Idle;
    static constexpr int DummyInitialDelay = 35;
    static constexpr int DummyAttackInterval = 95;
    int m_dummyAttackDelay = DummyInitialDelay;
    qint64 m_trainingPlayerRefillAt = 0;
    int m_lastDamage = 0;
    int m_totalDamage = 0;
    qint64 m_dummyRefillAt = 0;
    bool m_showHitboxes = false;
    QRect m_lastHitBox;
    int m_hitboxFlashTicks = 0;
    HitSpark m_p1HitSpark;
    HitSpark m_p2HitSpark;
    static constexpr int HitSparkTicks = 10;

    // 顿帧（hitstop）：命中瞬间冻结双方逻辑若干帧，打击感的第一来源
    int m_hitStopTicks = 0;
    int m_shakeTicks = 0;
    int m_shakeAmplitude = 0;
    static constexpr int ShakeTicks = 6;

    bool p1Left = false, p1Right = false, p2Left = false, p2Right = false;
    // 组合技：跳跃/快速下落键按住时按攻击键 -> 跃击/下劈
    bool p1JumpHeld = false, p1FallHeld = false, p2JumpHeld = false, p2FallHeld = false;

    static constexpr double MOVE_SPD = 4.5;
    static constexpr double COMPUTER_MOVE_SPD = 3.5;
    static constexpr int COUNTDOWN_SECONDS = 3;
    static constexpr int ROUND_SECONDS = 60;
    static constexpr int START_BANNER_MS = 600;
};

#endif // MAINWINDOW_H
