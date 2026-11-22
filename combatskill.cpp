#include "combatskill.h"

#include "fighter.h"

SkillEffect SkillEffect::spawn(SkillKind kind, const Fighter &caster, bool enhanced)
{
    const QRect body = caster.getBodyBox();
    const int direction = caster.facingRight ? 1 : -1;
    switch (kind) {
    case SkillKind::SwordWave: {
        const int width = enhanced ? 92 : 62;
        const int height = enhanced ? 42 : 27;
        const int left = caster.facingRight ? body.right() + 3 : body.left() - width - 2;
        return {kind, QRect(left, body.center().y() - height / 2, width, height),
                direction, enhanced ? 32 : 38, false, enhanced};
    }
    case SkillKind::Lightning:
        if (enhanced)
            return {kind, QRect(body.center().x() + direction * 110 - 75, body.bottom() - 125,
                                150, 126), direction, EnhancedLightningTicks, false, true};
        return {kind, QRect(body.center().x() - 76, body.bottom() - 101, 152, 102), direction, LightningTicks};
    case SkillKind::Shadow:
    default:
        return {};
    }
}

SkillEffect SkillEffect::spawnWind(const Fighter &caster, WindGust gust)
{
    const QRect body = caster.getBodyBox();
    const auto spec = windGustSpec(gust);
    const int direction = caster.facingRight ? 1 : -1;
    const int left = direction > 0 ? body.center().x() + 7 : body.center().x() - 6 - spec.reach;
    return {SkillKind::Wind, QRect(left, body.top() - 5, spec.reach, body.height() + 10),
            direction, WindGustTicks, false, false, gust};
}

void SkillEffect::advance(int leftBoundary, int rightBoundary)
{
    if (lifetime <= 0) return;
    --lifetime;
    if (kind != SkillKind::SwordWave || hit) return;

    area.translate(direction * (enhanced ? 15 : 9), 0);
    if (area.right() < leftBoundary || area.left() > rightBoundary) lifetime = 0;
}

void SkillEffect::markHit()
{
    hit = true;
    if (kind == SkillKind::SwordWave) lifetime = ImpactTicks;
}
