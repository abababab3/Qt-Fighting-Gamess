#include "playerlogic.h"

bool PlayerLogic::update()
{
    if (!m_f) return false;
    if (m_attackCooldown > 0) --m_attackCooldown;
    if (m_skillCooldown > 0) --m_skillCooldown;
    if (m_hurtTicks > 0) --m_hurtTicks;
    if (m_blockTicks > 0) --m_blockTicks;
    if (m_perfectGuardTicks > 0) --m_perfectGuardTicks;
    if (m_guardRearmTicks > 0) --m_guardRearmTicks;
    if (m_counterReadyTicks > 0) --m_counterReadyTicks;
    if (m_perfectFlashTicks > 0) --m_perfectFlashTicks;

    bool releaseSkill = false;
    if (isCasting()) {
        const SkillSpec spec = skillSpec(m_skill);
        releaseSkill = m_castTick == spec.startup;
        if (++m_castTick > spec.duration) m_castTick = 0;
    }
    if (m_attackTicks > 0) --m_attackTicks;
    updateStance();
    return releaseSkill;
}

void PlayerLogic::updateStance()
{
    if (!m_f || m_f->hp <= 0 || isCommittedToAttack() || isLunging() || isCountering() || isCasting() || m_hurtTicks > 0) return;

    if (m_f->onGround && (m_guardHeld || m_blockTicks > 0)) {
        m_attackTicks = 0;
        m_f->setState(AnimState::Guard);
    } else if (isPlayingAttack()) {
        return;
    } else if (!m_f->onGround)
        m_f->setState(AnimState::Jump);
    else
        m_f->setState(m_f->vx != 0 ? AnimState::Walk : AnimState::Idle);
}

bool PlayerLogic::isGuarding() const
{
    return m_f && m_f->hp > 0 && m_f->onGround && m_f->state() == AnimState::Guard;
}

bool PlayerLogic::canGuard() const
{
    return m_f && m_f->hp > 0 && m_f->onGround && !isCommittedToAttack()
           && !isCasting() && !isLunging() && !isCountering() && m_hurtTicks == 0;
}

bool PlayerLogic::canAttack() const
{
    return m_f && m_f->hp > 0 && !isCommittedToAttack() && m_attackCooldown == 0
           && m_hurtTicks == 0 && !isCasting() && !isLunging() && !isCountering() && !isGuarding();
}

bool PlayerLogic::canBlink() const
{
    return m_f && m_f->hp > 0 && m_hurtTicks == 0 && !isGuarding()
           && !isCasting() && !isLunging() && !isCountering() && !isCommittedToAttack();
}

bool PlayerLogic::canUseSkill() const
{
    return m_skill != SkillKind::None && m_skillCooldown == 0 && m_f && m_f->onGround
           && canAttack();
}

void PlayerLogic::setGuardHeld(bool held)
{
    if (m_guardHeld == held) return;
    m_guardHeld = held;
    if (!held) {
        m_perfectGuardTicks = 0;
    } else if (canGuard()) {
        m_f->vx = 0;
        if (m_skill != SkillKind::None && m_guardRearmTicks == 0 && m_blockTicks == 0) {
            // The counter is decremented before the next collision check.
            m_perfectGuardTicks = PerfectGuardTicks + 1;
            m_guardRearmTicks = GuardRearmTicks;
        }
    }
    updateStance();
}

bool PlayerLogic::attack()
{
    if (!canAttack()) return false;

    const AnimState attackState = nextAttackState();
    m_useAttack2 = !m_useAttack2;
    int frames = m_f->getFrameCount(attackState);
    if (frames <= 0) frames = attackState == AnimState::Attack2 ? 7 : 6;
    m_attackTicks = frames * 6 + 6;
    m_attackCooldown = AttackCooldown;
    m_attackConnected = false;
    m_f->setState(attackState);
    return true;
}

bool PlayerLogic::lunge(int direction)
{
    if (!canBlink()) return false;

    const LungeSpec spec = lungeSpec(m_skill);
    m_attackTicks = spec.duration;
    m_attackCooldown = spec.duration;
    m_attackConnected = false;
    m_f->setFacing(direction > 0);
    m_f->setState(AnimState::Lunge);
    return true;
}

bool PlayerLogic::counter()
{
    if (m_counterReadyTicks <= 0 || !m_f || m_f->hp <= 0 || m_hurtTicks > 0 ||
        isCasting() || isLunging() || isCountering() || isCommittedToAttack()) return false;

    const auto spec = counterSpec(m_skill);
    clearCounterOpportunity();
    m_blockTicks = 0;
    m_attackTicks = m_attackCooldown = spec.duration;
    m_attackConnected = false;
    m_f->setState(AnimState::Counter);
    return true;
}

void PlayerLogic::clearCounterOpportunity()
{
    m_perfectGuardTicks = 0;
    m_counterReadyTicks = 0;
}

int PlayerLogic::counterTick() const
{
    return counterSpec(m_skill).duration - m_attackTicks;
}

bool PlayerLogic::hasCounterArmor() const
{
    if (!isCountering()) return false;
    const auto spec = counterSpec(m_skill);
    return counterTick() <= spec.windup + spec.active;
}

int PlayerLogic::lungeTick() const
{
    return lungeSpec(m_skill).duration - m_attackTicks;
}

double PlayerLogic::lungeVelocity() const
{
    const auto spec = lungeSpec(m_skill);
    const int tick = lungeTick();
    if (tick < spec.windup || tick >= spec.windup + spec.travel ||
        (m_skill == SkillKind::Lightning && m_attackConnected)) return 0;
    return (m_f->facingRight ? 1 : -1) * double(spec.distance) / spec.travel;
}

bool PlayerLogic::useSkill()
{
    if (!canUseSkill()) return false;
    m_attackTicks = 0;
    m_skillCooldown = skillSpec(m_skill).cooldown;
    m_castTick = 1;
    m_f->setState(AnimState::Skill);
    return true;
}

QRect PlayerLogic::activeAttackBox() const
{
    if (!m_f || !isCommittedToAttack()) return {};
    int reach = 0;
    if (isLunging()) {
        const auto spec = lungeSpec(m_skill);
        const int tick = lungeTick();
        if (tick <= spec.windup || tick > spec.windup + spec.travel) return {};
        reach = spec.reach;
    } else if (isCountering()) {
        const auto spec = counterSpec(m_skill);
        const int tick = counterTick();
        if (tick <= spec.windup || tick > spec.windup + spec.active) return {};
        reach = spec.reach;
    } else {
        // The training dummy has a fixed pose but still needs a readable wind-up.
        if (m_f->getFrameCount(m_f->state()) == 0 && (m_attackTicks > 24 || m_attackTicks < 18))
            return {};
        return m_f->getHitBox();
    }

    const QRect body = m_f->getBodyBox();
    const int top = body.top() + body.height() / 5;
    const int height = body.height() * 3 / 5;
    const int left = m_f->facingRight ? body.center().x() + 3 : body.center().x() - reach - 2;
    return QRect(left, top, reach, height);
}

HitResult PlayerLogic::resolveHit(PlayerLogic &target)
{
    if (activeAttackBox().isEmpty() || !target.m_f || target.m_f->hp <= 0) return {};

    m_attackConnected = true;
    const int damage = isCountering() ? counterSpec(m_skill).damage
                                      : isLunging() ? lungeSpec(m_skill).damage : AttackDamage;
    return target.receiveHit(damage, m_f->getBodyBox().center().x());
}

HitResult PlayerLogic::receiveHit(int damage, int sourceX)
{
    if (!m_f || m_f->hp <= 0 || damage <= 0) return {};
    const int previousHp = m_f->hp;
    const int offset = sourceX - m_f->getBodyBox().center().x();
    const bool fromFront = m_f->facingRight ? offset > 0 : offset < 0;
    if (isGuarding() && fromFront) {
        if (m_perfectGuardTicks > 0) {
            m_perfectGuardTicks = 0;
            m_counterReadyTicks = CounterReadyTicks;
            m_perfectFlashTicks = PerfectFlashTicks;
            return {0, true, true};
        }
        m_f->hp -= qMin(m_f->hp - 1, (damage + 3) / 4);
        m_f->blockFlashTicks = Fighter::HitFlashTicks;
        m_blockTicks = BlockTicks;
        return {previousHp - m_f->hp, true};
    }

    if (hasCounterArmor() && damage < previousHp) {
        m_f->hp -= damage;
    } else {
        applyDamage(damage);
    }
    return {previousHp - m_f->hp, false};
}

void PlayerLogic::applyDamage(int damage)
{
    m_f->hp = qMax(0, m_f->hp - damage);
    m_f->flashTicks = Fighter::HitFlashTicks;
    m_f->blockFlashTicks = 0;
    m_hurtTicks = HurtTicks;
    clearCounterOpportunity();
    m_attackTicks = m_blockTicks = m_castTick = 0;
    m_f->setState(AnimState::Hurt);
}
