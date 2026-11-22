#include "ninja.h"

#include "fighter.h"
#include "stage.h"

NinjaProjectile NinjaProjectile::launch(NinjaShot kind, const QRect &body, int direction)
{
    const auto spec = ninjaShotSpec(kind);
    const int left = direction > 0 ? body.right() + 1 : body.left() - spec.width;
    return {kind, QRect(left, body.center().y() - spec.height / 2, spec.width, spec.height),
            direction, spec.lifetime};
}

void NinjaProjectile::advance()
{
    if (ticks <= 0) return;
    --ticks;
    area.translate(direction * ninjaShotSpec(kind).speed, 0);
    if (area.right() < 0 || area.left() >= Stage::Width) ticks = 0;
}

void NinjaKit::placeClone(const Fighter &fighter)
{
    m_clone = m_plannedClone.isValid() ? m_plannedClone : fighter.getBodyBox();
    m_plannedClone = {};
    m_cloneDirection = fighter.facingRight ? 1 : -1;
    m_cloneTicks = CloneTicks;
    m_echoTicks = 0;
    m_smokeTicks = 0;
}

void NinjaKit::commitEcho(const Fighter &target, int fallbackDirection)
{
    if (!hasClone() || echoPending()) return;
    const int offset = target.getBodyBox().center().x() - m_clone.center().x();
    m_cloneDirection = offset != 0 ? (offset > 0 ? 1 : -1) : fallbackDirection;
    m_echoTicks = NinjaThrowWindup + EchoDelay;
    m_cloneTicks = qMax(m_cloneTicks, m_echoTicks);
}

bool NinjaKit::swap(Fighter &fighter, const Stage &stage)
{
    if (!hasClone() || echoPending()) return false;
    const QRect origin = fighter.getBodyBox();
    StagePhysics::relocate(fighter, m_clone, stage);
    fadeClone(origin);
    return true;
}

void NinjaKit::disrupt(const QRect &attack)
{
    if (hasClone() && attack.intersects(m_clone)) fadeClone();
}

void NinjaKit::fadeClone(const QRect &destination)
{
    m_smokeFrom = m_clone;
    m_smokeTo = destination;
    m_smokeTicks = SmokeTicks;
    m_cloneTicks = m_echoTicks = 0;
}

void NinjaKit::advance()
{
    own.advance();
    echo.advance();
    if (m_smokeTicks > 0) --m_smokeTicks;
    if (!hasClone()) return;
    --m_cloneTicks;
    if (m_echoTicks > 0 && --m_echoTicks == 0) {
        echo = NinjaProjectile::launch(NinjaShot::Echo, m_clone, m_cloneDirection);
        fadeClone();
    } else if (m_cloneTicks == 0) {
        fadeClone();
    }
}
