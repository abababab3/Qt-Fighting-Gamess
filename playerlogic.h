#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

#include "combatskill.h"
#include "fighter.h"

struct HitResult {
    int damage = 0;
    bool blocked = false;
    bool perfect = false;
    bool guardBroke = false;   // 这一击正好把盾打碎
};

class PlayerLogic {
public:
    explicit PlayerLogic(Fighter *fighter = nullptr, SkillKind skill = SkillKind::None)
        : m_f(fighter), m_skill(skill) {}

    // Returns true on the tick that a skill effect should be created.
    bool update();
    bool attack();
    bool lunge(int direction);
    bool counter();
    QRect activeAttackBox() const;
    HitResult resolveHit(PlayerLogic &target);
    // guardDamage：这一招对护盾的削减量。<=0 时按 伤害×200% 兜底推导。
    // knockback：这一招的击退初速。<=0 时按 伤害 兜底推导。
    HitResult receiveHit(int damage, int guardDamage, int sourceX, double knockback = 0);
    bool useSkill();
    void setGuardHeld(bool held);

    bool isGuarding() const;

    // ---- 护盾 / 破防 ----
    // 格挡不再无限：每次挡下攻击都扣护盾，护盾见底即破防 + 长硬直。
    // 完美格挡不扣护盾，反而小幅回盾，形成"主动去拼完美格挡"的正向激励。
    // ---- 击退 ----
    // 受击后有一小段"被推走"的窗口，期间 vx 由惯性衰减控制，按键抢不回来。
    // 这是打击感里"卡一下→弹开"的后半截，也让连招需要主动追步。
    bool isKnockedBack() const { return m_knockTicks > 0; }
    // 每帧在物理步进前调用。击退 / 破防硬直期间接管速度（惯性衰减），
    // 返回 true 表示本帧移动由逻辑接管，调用方不要再按输入改写 vx。
    bool overrideMovement(Fighter &fighter) const;
    void clearKnockback() { m_knockTicks = 0; }
    static constexpr int KnockTicks = 10;            // 击退窗口（约 0.16s）
    static constexpr double KnockDecay = 0.82;       // 每帧衰减，10帧后基本停住

    bool isGuardBroken() const { return m_guardBrokenTicks > 0; }
    int  guardValue() const { return m_guard; }
    int  guardMax() const { return GuardMax; }
    int  guardBrokenTicks() const { return m_guardBrokenTicks; }
    int  guardBlockFlash() const { return m_blockFlashTicks; }
    int  guardBreakFlash() const { return m_breakFlashTicks; }
    void resetGuard();

    static constexpr int GuardMax = 100;             // 护盾上限
    static constexpr int AttackGuardDamage = 20;     // 普通攻击破盾值（约 6 下拆满）
    static constexpr double AttackKnockback = 4.0;   // 普攻击退初速（约 15px 位移）
    static constexpr int ChipPct = 25;               // 格挡仍吃 25% 削减伤害
    static constexpr int GuardRegenEvery = 8;        // 每 8 tick 回 1 点（约 12.8s 回满）
    static constexpr int GuardBreakTicks = 56;       // 破防硬直（约 0.9s，含追击时间仍够打 3 下）
    static constexpr int GuardBreakFlashTicks = 22;

    bool isCasting() const { return m_castTick > 0; }
    bool isPlayingAttack() const { return m_attackTicks > 0; }
    bool isLunging() const { return m_f && m_attackTicks > 0 && m_f->state() == AnimState::Lunge; }
    bool isCountering() const { return m_f && m_attackTicks > 0 && m_f->state() == AnimState::Counter; }
    bool hasCounterArmor() const;
    int counterTick() const;
    int counterReadyTicks() const { return m_counterReadyTicks; }
    int perfectFlashTicks() const { return m_perfectFlashTicks; }
    void clearCounterOpportunity();
    int lungeTick() const;
    double lungeVelocity() const;
    bool isCommittedToAttack() const { return isPlayingAttack() && !m_attackConnected; }
    bool canGuard() const;
    bool canAttack() const;
    bool canUseSkill() const;
    bool canBlink() const;
    SkillKind skill() const { return m_skill; }
    int skillCooldown() const { return m_skillCooldown; }
    static constexpr int CounterReadyTicks = 36;
    static constexpr int PerfectFlashTicks = 13;
    AnimState nextAttackState() const { return m_useAttack2 ? AnimState::Attack2 : AnimState::Attack; }

private:
    void updateStance();
    void applyDamage(int damage, int knockDir, double knockSpeed);
    void applyKnockback(int direction, double speed);
    void guardBreak();
    void regenGuard();

    Fighter *m_f = nullptr;
    SkillKind m_skill = SkillKind::None;
    int m_attackCooldown = 0;
    int m_attackTicks = 0;
    int m_hurtTicks = 0;
    int m_knockTicks = 0;
    int m_blockTicks = 0;
    int m_perfectGuardTicks = 0;
    int m_guardRearmTicks = 0;
    int m_counterReadyTicks = 0;
    int m_perfectFlashTicks = 0;
    int m_skillCooldown = 0;
    int m_castTick = 0;
    int m_guard = GuardMax;
    int m_guardBrokenTicks = 0;
    int m_guardRegenCounter = 0;
    int m_blockFlashTicks = 0;
    int m_breakFlashTicks = 0;
    bool m_guardHeld = false;
    bool m_useAttack2 = false;
    bool m_attackConnected = false;

    static constexpr int AttackCooldown = 24;
    static constexpr int AttackDamage = 8;
    static constexpr int HurtTicks = 12;
    static constexpr int BlockTicks = 8;
    static constexpr int PerfectGuardTicks = 6;
    static constexpr int GuardRearmTicks = 18;
};

#endif // PLAYERLOGIC_H
