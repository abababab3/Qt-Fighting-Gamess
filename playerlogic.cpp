#include "playerlogic.h"

bool PlayerLogic::update()
{
    if (!m_f) return false;
    if (m_attackCooldown > 0) --m_attackCooldown;
    if (m_skillCooldown > 0) --m_skillCooldown;
    if (m_hurtTicks > 0) --m_hurtTicks;
    if (m_knockTicks > 0) {
        --m_knockTicks;
        // 窗口结束时把惯性归零，否则会残留一点速度让角色慢慢滑
        if (m_knockTicks == 0 && m_f) m_f->vx = 0;
    }
    if (m_blockTicks > 0) --m_blockTicks;
    if (m_perfectGuardTicks > 0) --m_perfectGuardTicks;
    if (m_guardRearmTicks > 0) --m_guardRearmTicks;
    if (m_counterReadyTicks > 0) --m_counterReadyTicks;
    if (m_perfectFlashTicks > 0) --m_perfectFlashTicks;
    if (m_guardBrokenTicks > 0) --m_guardBrokenTicks;
    if (m_blockFlashTicks > 0) --m_blockFlashTicks;
    if (m_breakFlashTicks > 0) --m_breakFlashTicks;
    regenGuard();

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
    // 破防硬直期间锁死姿势：不许举盾、不许走位，等硬直读完再恢复控制
    if (m_guardBrokenTicks > 0) return;

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
           && !isCasting() && !isLunging() && !isCountering() && m_hurtTicks == 0
           && m_guardBrokenTicks == 0;
}

bool PlayerLogic::canAttack() const
{
    return m_f && m_f->hp > 0 && !isCommittedToAttack() && m_attackCooldown == 0
           && m_hurtTicks == 0 && !isCasting() && !isLunging() && !isCountering() && !isGuarding()
           && m_guardBrokenTicks == 0;
}

bool PlayerLogic::canBlink() const
{
    return m_f && m_f->hp > 0 && m_hurtTicks == 0 && !isGuarding()
           && !isCasting() && !isLunging() && !isCountering() && !isCommittedToAttack()
           && m_guardBrokenTicks == 0;
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

namespace {
int slamFrames(Fighter *f, AnimState state, int fallback)
{
    int frames = f ? f->getFrameCount(state) : 0;
    return frames > 0 ? frames : fallback;
}
} // namespace

bool PlayerLogic::airSlash()
{
    if (!canAttack()) return false;
    const AnimState state = AnimState::AirSlash;
    const int frames = slamFrames(m_f, state, 6);
    // 跃击动画在 Fighter 里按 FRAME_DELAY*2 播放，状态计时要同步，
    // 否则 m_attackTicks 归零后 updateStance 会把动画提前切回待机(只播前几帧)。
    m_attackTicks = frames * 12 + 10;
    m_attackCooldown = AttackCooldown;
    m_attackConnected = false;
    m_f->setState(state);
    return true;
}

bool PlayerLogic::downSlash()
{
    if (!canAttack()) return false;
    const AnimState state = AnimState::DownSlash;
    const int frames = slamFrames(m_f, state, 6);
    // 下劈 4 帧按 FRAME_DELAY(6) 播放，留足落地帧(最后一帧)的停留时间。
    m_attackTicks = frames * 6 + 14;
    m_attackCooldown = AttackCooldown + 6;
    m_attackConnected = false;
    m_f->setState(state);
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
    int damage = isCountering() ? counterSpec(m_skill).damage
                                : isLunging() ? lungeSpec(m_skill).damage : AttackDamage;
    int guardDamage = isCountering() ? counterSpec(m_skill).guardDamage
                                     : isLunging() ? lungeSpec(m_skill).guardDamage
                                                   : AttackGuardDamage;
    double knockback = isCountering() ? counterSpec(m_skill).knockback
                                      : isLunging() ? lungeSpec(m_skill).knockback
                                                    : AttackKnockback;
    // 跃击 / 下劈 = 重攻击，统一 10 点伤害（普攻 8）；破盾与击退略高于普攻。
    if (m_f->state() == AnimState::AirSlash || m_f->state() == AnimState::DownSlash) {
        damage = 10; guardDamage = 24; knockback = 4.6;
    }
    return target.receiveHit(damage, guardDamage, m_f->getBodyBox().center().x(), knockback);
}

HitResult PlayerLogic::receiveHit(int damage, int guardDamage, int sourceX, double knockback)
{
    if (!m_f || m_f->hp <= 0 || damage <= 0) return {};
    const int previousHp = m_f->hp;
    const int bodyX = m_f->getBodyBox().center().x();
    const int offset = sourceX - bodyX;
    const bool fromFront = m_f->facingRight ? offset > 0 : offset < 0;
    // 击退方向永远背离攻击来源，跟朝向无关（被从背后打也是往前扑出去）
    const int knockDir = sourceX >= bodyX ? -1 : 1;
    // 未标定击退值的调用方（旧代码 / 测试）走伤害兜底，保证不会出现零击退
    const double knockSpeed = knockback > 0 ? knockback : 4.2 + damage * 0.12;
    if (isGuarding() && fromFront) {
        if (m_perfectGuardTicks > 0) {
            m_perfectGuardTicks = 0;
            m_counterReadyTicks = CounterReadyTicks;
            m_perfectFlashTicks = PerfectFlashTicks;
            m_guard = qMin(GuardMax, m_guard + GuardMax / 20);   // 完美格挡回 5 点盾
            return {0, true, true};
        }

        // 普通格挡：吃 25% 削减伤害，同时按 200% 消耗护盾
        int chip = damage * ChipPct / 100;
        if (chip < 1) chip = 1;
        chip = qMin(chip, qMax(0, m_f->hp - 1));    // 格挡本身绝不致死，最多留 1 血
        m_f->hp -= chip;

        m_guard -= guardDamage > 0 ? guardDamage : damage * 2;
        m_f->blockFlashTicks = Fighter::HitFlashTicks;
        m_blockFlashTicks = 12;
        if (m_guard <= 0) {
            guardBreak();                            // 盾碎：进长硬直，这段时间任你打
            applyKnockback(knockDir, knockSpeed + 1.4);   // 破防比普通受击弹得更远
            return {previousHp - m_f->hp, true, false, true};
        }
        m_blockTicks = BlockTicks;
        applyKnockback(knockDir, 2.2);               // 举盾被推，只挪一点，不至于被推出立回
        return {previousHp - m_f->hp, true};
    }

    if (hasCounterArmor() && damage < previousHp) {
        m_f->hp -= damage;
    } else {
        applyDamage(damage, knockDir, knockSpeed);
    }
    return {previousHp - m_f->hp, false};
}

void PlayerLogic::applyKnockback(int direction, double speed)
{
    if (!m_f) return;
    m_f->vx = direction * speed;
    m_knockTicks = KnockTicks;
}

bool PlayerLogic::overrideMovement(Fighter &fighter) const
{
    // 破防硬直比击退窗口长：即使惯性已耗尽，也不能靠按键提前恢复走位
    if (m_knockTicks <= 0 && m_guardBrokenTicks <= 0) return false;
    if (m_knockTicks > 0) {
        fighter.vx *= KnockDecay;
        if (qAbs(fighter.vx) < 0.6) fighter.vx = 0;
    } else {
        fighter.vx = 0;      // 惯性已耗尽，破防硬直期间原地不动
    }
    return true;
}

void PlayerLogic::applyDamage(int damage, int knockDir, double knockSpeed)
{
    m_f->hp = qMax(0, m_f->hp - damage);
    m_f->flashTicks = Fighter::HitFlashTicks;
    m_f->blockFlashTicks = 0;
    m_hurtTicks = HurtTicks;
    clearCounterOpportunity();
    m_attackTicks = m_blockTicks = m_castTick = 0;
    m_f->setState(AnimState::Hurt);
    applyKnockback(knockDir, knockSpeed);
}

void PlayerLogic::guardBreak()
{
    m_guard = 0;
    m_guardBrokenTicks = GuardBreakTicks;
    m_breakFlashTicks = GuardBreakFlashTicks;
    m_guardRegenCounter = 0;

    // 强制卸盾：即使手指还按着格挡键，也必须松开重按才能重新举盾
    m_guardHeld = false;
    m_blockTicks = 0;
    m_perfectGuardTicks = 0;
    m_guardRearmTicks = 0;
    m_perfectFlashTicks = 0;
    clearCounterOpportunity();

    // 打断一切动作，进 Hurt 姿势。位移由调用方的 applyKnockback 负责，这里只管状态
    m_attackTicks = 0;
    m_castTick = 0;
    m_hurtTicks = 0;
    if (m_f) m_f->setState(AnimState::Hurt);
}

void PlayerLogic::regenGuard()
{
    if (m_guard >= GuardMax) return;
    if (m_guardBrokenTicks > 0) return;      // 硬直期间不回盾，破防才真的有代价
    if (++m_guardRegenCounter < GuardRegenEvery) return;
    m_guardRegenCounter = 0;
    ++m_guard;
}

void PlayerLogic::resetGuard()
{
    m_guard = GuardMax;
    m_guardBrokenTicks = 0;
    m_guardRegenCounter = 0;
    m_blockFlashTicks = 0;
    m_breakFlashTicks = 0;
}
