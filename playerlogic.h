#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

#include "combatskill.h"
#include "fighter.h"

struct HitResult {
    int damage = 0;
    bool blocked = false;
    bool perfect = false;
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
    HitResult receiveHit(int damage, int sourceX);
    bool useSkill();
    void setGuardHeld(bool held);

    bool isGuarding() const;
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
    void applyDamage(int damage);

    Fighter *m_f = nullptr;
    SkillKind m_skill = SkillKind::None;
    int m_attackCooldown = 0;
    int m_attackTicks = 0;
    int m_hurtTicks = 0;
    int m_blockTicks = 0;
    int m_perfectGuardTicks = 0;
    int m_guardRearmTicks = 0;
    int m_counterReadyTicks = 0;
    int m_perfectFlashTicks = 0;
    int m_skillCooldown = 0;
    int m_castTick = 0;
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
