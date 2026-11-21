#ifndef GAMEUI_H
#define GAMEUI_H

#include <QPainter>

#include "combatskill.h"
#include "stage.h"

class Blink;
class Fighter;
class PlayerLogic;

namespace GameUI {

constexpr int Width = Stage::Width;
constexpr int Height = 600;
constexpr int GroundY = Stage::GroundY;

inline const QRect MenuPrimaryButton(52, 221, 290, 52);
inline const QRect MenuSecondaryButton(52, 286, 290, 52);
inline const QRect MenuTrainingButton(52, 351, 290, 52);
inline const QRect MenuKeySettingsButton(52, 416, 290, 52);
inline const QRect StageSelector(102, 27, 194, 32);
inline const QRect MusicMuteButton(556, 27, 94, 32);
inline const QRect MusicVolumeSlider(662, 27, 82, 32);
inline const QRect ResultPrimaryButton(174, 341, 218, 48);
inline const QRect ResultSecondaryButton(408, 341, 218, 48);

enum class Winner { Draw, PlayerOne, PlayerTwo };

class HealthTrail {
public:
    void reset(int hp);
    void update(int hp, qint64 elapsed);
    qreal value(qint64 elapsed) const;

private:
    int m_hp = 0;
    qreal m_previous = 0;
    qint64 m_hitAt = 0;
};

struct HealthBar {
    int hp;
    int maxHp;
    qreal trailHp;
    // 护盾（破防系统）：guardMax<=0 时不绘制护盾条
    int guard = 0;
    int guardMax = 0;
    bool guardBroken = false;
    int guardBrokenTicks = 0;
};

void drawArena(QPainter &p, const Stage &stage);
void drawMenu(QPainter &p, const Fighter &playerOne, const Fighter &playerTwo, const Stage &stage, bool adjustingVolume);
void drawHud(QPainter &p, const HealthBar &p1, const HealthBar &p2, int remainingMs, bool showMenuHint,
             const QString &opponentName);
void drawTrainingHud(QPainter &p, const HealthBar &p1, const HealthBar &dummy, int lastDamage, int totalDamage, const QString &dummyName);
void drawTrainingDummy(QPainter &p, const QRect &body, int health, qreal flash,
                       bool facingRight, bool winding, const QRect &attackBox);
void drawHitboxes(QPainter &p, const QRect &p1Body, const QRect &p2Body,
                  const QRect &p1Attack, const QRect &p2Attack, const QRect &lastHit);
void drawBlinkTrail(QPainter &p, const Blink &blink, bool firstPlayer);
void drawBlinkStatus(QPainter &p, const Blink &p1, const Blink &p2, bool showP2);
void drawLungeEffect(QPainter &p, const Fighter &fighter, SkillKind kind, int elapsed);
void drawGuardEffect(QPainter &p, const Fighter &fighter, bool guarding);
void drawCounterEffects(QPainter &p, const Fighter &fighter, const PlayerLogic &logic);
void drawCounterStatus(QPainter &p, const PlayerLogic &p1, const PlayerLogic &p2, bool showP2);
void drawSkillEffect(QPainter &p, const SkillEffect &effect);
void drawHitSpark(QPainter &p, const QPoint &contact, qreal age);
void drawSkillHitboxes(QPainter &p, const QRect &p1Skill, const QRect &p2Skill);
void drawSkillStatus(QPainter &p, SkillKind p1, int p1Cooldown, SkillKind p2, int p2Cooldown);
void drawRoundStart(QPainter &p, int countdown);
void drawResult(QPainter &p, Winner winner, const QString &detail, const QString &opponentName);

} // namespace GameUI

#endif // GAMEUI_H
