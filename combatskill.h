#ifndef COMBATSKILL_H
#define COMBATSKILL_H

#include <QRect>

class Fighter;

enum class SkillKind { None, SwordWave, Lightning, Shadow, Wind };

enum class NinjaShot { None, Kunai, Echo, Shuriken, Counter };
inline constexpr int NinjaThrowWindup = 9;

struct NinjaShotSpec {
    int damage;
    int guardDamage;
    double knockback;
    int speed;
    int lifetime;
    int width;
    int height;
};

constexpr NinjaShotSpec ninjaShotSpec(NinjaShot shot)
{
    switch (shot) {
    case NinjaShot::Kunai: return {6, 11, 3.2, 14, 31, 24, 10};
    case NinjaShot::Echo: return {4, 7, 2.3, 14, 31, 22, 10};
    case NinjaShot::Shuriken: return {10, 17, 4.6, 17, 24, 32, 27};
    case NinjaShot::Counter: return {13, 26, 5.6, 21, 6, 42, 22};
    default: return {};
    }
}

struct SkillSpec {
    int startup;
    int duration;
    int cooldown;
    int damage;
    int guardDamage;   // 破盾值：独立于伤害标定，重招拆盾快
    double knockback;  // 击退初速：同样独立标定，重招把人打得更远
};

inline constexpr int CombatTickMs = 16;

constexpr SkillSpec skillSpec(SkillKind kind, bool enhanced = false)
{
    switch (kind) {
    case SkillKind::SwordWave: return enhanced ? SkillSpec{7, 26, 300, 23, 41, 7.4} : SkillSpec{11, 29, 300, 18, 34, 6.6};
    case SkillKind::Lightning: return enhanced ? SkillSpec{11, 32, 375, 28, 64, 8.3} : SkillSpec{15, 34, 375, 24, 56, 7.6};
    case SkillKind::Shadow: return {5, 15, 344, 0, 0, 0};
    case SkillKind::Wind: return {0, 0, 250, 0, 0, 0};
    default: return {};
    }
}

enum class WindGust { None, Push, ChargedPush, Pull };

struct WindGustSpec {
    int damage;
    int guardDamage;
    double force;
    int reach;
};

constexpr WindGustSpec windGustSpec(WindGust gust)
{
    switch (gust) {
    case WindGust::Push: return {6, 10, 4.8, 85};
    case WindGust::ChargedPush: return {12, 19, 7.9, 102};
    case WindGust::Pull: return {8, 14, 6.0, 111};
    default: return {};
    }
}

inline constexpr int WindHoldThreshold = 8;
inline constexpr int WindCatchTicks = 11;
inline constexpr int WindMaxHoldTicks = 28;
inline constexpr int WindGlideTicks = 50;
inline constexpr int WindLungeStrikeTicks = 5;
inline constexpr double WindFallSpeed = 2.2;

struct WindNormalSpec {
    int startup;
    int active;
    int duration;
    int damage;
    int reach;
    int guardDamage;
    double knockback;
};

constexpr WindNormalSpec windNormalSpec(bool sweep)
{
    return sweep ? WindNormalSpec{11, 6, 30, 9, 58, 19, 4.3}
                 : WindNormalSpec{8, 5, 25, 7, 47, 15, 3.5};
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
    if (kind == SkillKind::Wind) return {3, 6, 26, 68, 12, 59, 26, 5.2};
    if (kind == SkillKind::Shadow) return {6, 4, 20, 20, 10, 0, 10, 4.6};
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
    if (kind == SkillKind::Wind) return {5, 6, 24, 14, 58, 32, 5.7};
    if (kind == SkillKind::Shadow) return {4, 6, 23, 13, 0, 26, 5.6};
    if (kind == SkillKind::Lightning) return {6, 6, 28, 18, 70, 46, 8.4};
    return {4, 5, 23, 15, 64, 38, 6.0};
}

inline constexpr int KnightCounterStepTicks = 4;
inline constexpr double KnightCounterStepSpeed = 7.0;

struct SkillEffect {
    SkillKind kind = SkillKind::None;
    QRect area;
    int direction = 0;
    int lifetime = 0;
    bool hit = false;
    bool enhanced = false;
    WindGust wind = WindGust::None;

    static SkillEffect spawn(SkillKind kind, const Fighter &caster, bool enhanced = false);
    static constexpr int WindGustTicks = 9;
    static SkillEffect spawnWind(const Fighter &caster, WindGust gust);
    static constexpr int ImpactTicks = 9;
    static constexpr int LightningTicks = 9;
    static constexpr int EnhancedLightningTicks = 12;
    void advance(int leftBoundary, int rightBoundary);
    void markHit();
    QRect hitBox() const { return lifetime > 0 && !hit ? area : QRect(); }
};

#endif // COMBATSKILL_H
