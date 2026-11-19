#ifndef FIGHTER_H
#define FIGHTER_H

#include <QPixmap>
#include <QRect>
#include <QColor>
#include <QPainter>

enum class AnimState { Idle, Walk, Jump, Attack, Hurt };

class Fighter {
public:
    Fighter();

    // ---- 组员A 负责 ----
    bool loadFrames(AnimState state, const QString &path,
                    int frameCount, int fw, int fh);
    void setState(AnimState s);
    void updateAnimation();
    void draw(QPainter &p) const;

    // ---- 数据 ----
    double x = 0, y = 0, vx = 0, vy = 0;
    bool onGround = false;
    bool facingRight = true;
    int  hp = 100;
    int  maxHp = 100;
    QColor placeholderColor = Qt::darkCyan;

    QRect getBodyBox() const;
    QRect getHitBox()  const;
    bool  isAttackActive() const;

    void reset(double startX, double startY);
    void faceTo(const Fighter &other);

private:
    QPixmap m_sheet[5];
    int     m_frameCount[5] = {0,0,0,0,0};
    int     m_frameW[5]     = {80,80,80,80,80};
    int     m_frameH[5]     = {120,120,120,120,120};

    int       m_curFrame = 0;
    AnimState m_state = AnimState::Idle;
    int       m_tick = 0;
    static constexpr int FRAME_DELAY = 6;
};

#endif // FIGHTER_H