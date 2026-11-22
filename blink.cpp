#include "blink.h"

#include "fighter.h"
#include "stage.h"

bool Blink::tap(int direction, qint64 now, bool allowed, Fighter &fighter, const Stage &stage)
{
    if (!allowed || m_cooldown > 0) {
        clearTap();
        return false;
    }

    if (direction != m_tapDirection || now - m_lastTap > TapWindowMs) {
        m_tapDirection = direction;
        m_lastTap = now;
        return false;
    }

    clearTap();
    const QPoint start = fighter.getBodyBox().center();
    if (!StagePhysics::blink(fighter, direction * Distance, stage)) return false;

    m_trailStart = start;
    m_trailEnd = fighter.getBodyBox().center();
    m_trailTicks = TrailTicks;
    m_followUpTicks = FollowUpTicks;
    m_cooldown = m_cooldownLength = CooldownTicks;
    return true;
}

void Blink::spendSkillFollowUp()
{
    if (m_followUpTicks == 0) return;
    clearFollowUp();
    extendSkillCooldown();
}

void Blink::extendSkillCooldown()
{
    if (m_cooldown == 0 || m_cooldownLength != CooldownTicks) return;
    m_cooldownLength = 2 * CooldownTicks;
    m_cooldown += CooldownTicks;
}

void Blink::clearTap()
{
    m_tapDirection = 0;
}

void Blink::advance()
{
    if (m_cooldown > 0) --m_cooldown;
    if (m_trailTicks > 0) --m_trailTicks;
    if (m_followUpTicks > 0) --m_followUpTicks;
}
