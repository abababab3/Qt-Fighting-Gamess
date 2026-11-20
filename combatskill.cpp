#include "combatskill.h"

#include "fighter.h"

SkillEffect SkillEffect::spawn(SkillKind kind, const Fighter &caster)
{
    const QRect body = caster.getBodyBox();
    const int direction = caster.facingRight ? 1 : -1;
    switch (kind) {
    case SkillKind::SwordWave: {
        const int left = caster.facingRight ? body.right() + 3 : body.left() - 64;
        return {kind, QRect(left, body.center().y() - 13, 62, 27), direction, 38};
    }
    case SkillKind::Lightning:
        return {kind, QRect(body.center().x() - 76, body.bottom() - 101, 152, 102), direction, 9};
    default:
        return {};
    }
}

void SkillEffect::advance(int leftBoundary, int rightBoundary)
{
    if (lifetime <= 0) return;
    --lifetime;
    if (kind != SkillKind::SwordWave || hit) return;

    area.translate(direction * 9, 0);
    if (area.right() < leftBoundary || area.left() > rightBoundary) lifetime = 0;
}

void SkillEffect::markHit()
{
    hit = true;
    if (kind == SkillKind::SwordWave) lifetime = ImpactTicks;
}
