#ifndef FIGHTER_H
#define FIGHTER_H

#include <QPixmap>
#include <QRect>
#include <QColor>
#include <QPainter>

// Attack = 第一组攻击(attack1)，Attack2 = 第二组攻击(attack2)
// 每次按攻击键由 PlayerLogic 在两者间交替切换
enum class AnimState { Idle, Walk, Jump, Attack, Attack2, Hurt };
static constexpr int ANIM_STATE_COUNT = 6;

class Fighter {
public:
    Fighter();

    bool loadFrames(AnimState state, const QString &path,
                    int frameCount, int fw, int fh);
    void setState(AnimState s);
    AnimState state() const { return m_state; }   // 只读查询当前动画状态
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
    int   getFrameWidth() const;
    int   getFrameCount(AnimState s) const;
    bool  isAttackActive() const;

    void reset(double startX, double startY);
    void setFacing(bool right) { facingRight = right; }
    // 保留 faceTo 以兼容旧代码，但默认不再每帧调用（朝向改由移动键决定）
    void faceTo(const Fighter &other);

    // 受击闪白：命中瞬间设为 >0，每帧递减，>0 时绘制白色叠加
    int flashTicks = 0;

private:
    int  footGap() const;

    QPixmap m_sheet[ANIM_STATE_COUNT];
    int     m_frameCount[ANIM_STATE_COUNT] = {0,0,0,0,0,0};
    int     m_frameW[ANIM_STATE_COUNT]     = {80,80,80,80,80,80};
    int     m_frameH[ANIM_STATE_COUNT]     = {120,120,120,120,120,120};
    QRect   m_bodyRect[ANIM_STATE_COUNT];

    int       m_curFrame = 0;
    AnimState m_state = AnimState::Idle;
    int       m_tick = 0;
    static constexpr int FRAME_DELAY = 6;
};

#endif // FIGHTER_H
