#ifndef COMBATSKILL_H
#define COMBATSKILL_H

#include <QRect>

class Fighter;

enum class SkillKind { None, SwordWave, Lightning };

struct SkillSpec {
    int startup;
    int duration;
    int cooldown;
    int damage;
    int guardDamage;   // 破盾值：独立于伤害标定，重招拆盾快
    double knockback;  // 击退初速：同样独立标定，重招把人打得更远
};

inline constexpr int CombatTickMs = 16;

constexpr SkillSpec skillSpec(SkillKind kind)
{
    switch (kind) {
    case SkillKind::SwordWave: return {11, 29, 300, 18, 34, 6.6};
    case SkillKind::Lightning: return {15, 34, 375, 24, 56, 7.6};
    default: return {};
    }
}

struct LungeSpec {
    int windup;
    int travel;
    int duration;
    int distance;
    int damage;
    int reach;
    int guardDamage;
    double knockback;
};

constexpr LungeSpec lungeSpec(SkillKind kind)
{
    if (kind == SkillKind::Lightning) return {5, 6, 27, 72, 16, 49, 32, 6.4};
    return {2, 7, 22, 92, 13, 61, 26, 5.8};
}

struct CounterSpec {
    int windup;
    int active;
    int duration;
    int damage;
    int reach;
    int guardDamage;
    double knockback;
};

constexpr CounterSpec counterSpec(SkillKind kind)
{
    if (kind == SkillKind::Lightning) return {6, 6, 28, 18, 70, 38, 6.6};
    return {4, 5, 23, 15, 64, 38, 6.0};
}

struct SkillEffect {
    SkillKind kind = SkillKind::None;
    QRect area;
    int direction = 0;
    int lifetime = 0;
    bool hit = false;

    static SkillEffect spawn(SkillKind kind, const Fighter &caster);
    static constexpr int ImpactTicks = 9;
    void advance(int leftBoundary, int rightBoundary);
    void markHit();
    QRect hitBox() const { return lifetime > 0 && !hit ? area : QRect(); }
};

#endif // COMBATSKILL_H
