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
};

inline constexpr int CombatTickMs = 16;

constexpr SkillSpec skillSpec(SkillKind kind)
{
    switch (kind) {
    case SkillKind::SwordWave: return {11, 29, 300, 18};
    case SkillKind::Lightning: return {15, 34, 375, 24};
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
};

constexpr LungeSpec lungeSpec(SkillKind kind)
{
    if (kind == SkillKind::Lightning) return {5, 6, 27, 72, 16, 49};
    return {2, 7, 22, 92, 13, 61};
}

struct CounterSpec {
    int windup;
    int active;
    int duration;
    int damage;
    int reach;
};

constexpr CounterSpec counterSpec(SkillKind kind)
{
    if (kind == SkillKind::Lightning) return {6, 6, 28, 18, 70};
    return {4, 5, 23, 15, 64};
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
