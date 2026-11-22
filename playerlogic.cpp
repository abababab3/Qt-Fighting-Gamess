#include "playerlogic.h"

#include "stage.h"

#include <utility>

bool PlayerLogic::update()
{
    if (!m_f) return false;
    if (m_blinkInvulnerabilityTicks > 0) --m_blinkInvulnerabilityTicks;
    if (m_attackCooldown > 0) --m_attackCooldown;
    if (m_skillCooldown > 0) --m_skillCooldown;
    if (m_glideTicks > 0) --m_glideTicks;
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
        const SkillSpec spec = skillSpec(m_skill, m_enhancedCast);
        releaseSkill = m_castTick == spec.startup;
        if (++m_castTick > spec.duration) m_castTick = 0;
    }
    if (m_attackTicks > 0) --m_attackTicks;
    updateStance();
    updateWindInput();
    return releaseSkill;
}

void PlayerLogic::updateStance()
{
    if (!m_f || m_f->hp <= 0 || isCommittedToAttack() || isLunging() || isCountering() || isCasting() || isWindCharging() || m_hurtTicks > 0) return;
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
           && !isCasting() && !isWindCharging() && !isLunging() && !isCountering() && m_hurtTicks == 0
           && m_guardBrokenTicks == 0;
}

bool PlayerLogic::canAttack() const
{
    return m_f && m_f->hp > 0 && !isCommittedToAttack() && m_attackCooldown == 0
           && m_hurtTicks == 0 && !isCasting() && !isWindCharging() && !isLunging() && !isCountering() && !isGuarding()
           && m_guardBrokenTicks == 0;
}

bool PlayerLogic::isMovementStunned() const
{
    return m_hurtTicks > 0 || isKnockedBack() || isGuardBroken();
}

bool PlayerLogic::canControlMovement() const
{
    return m_f && m_f->hp > 0 && !isMovementStunned()
           && !isCasting() && !isWindCharging() && !isLunging() && !isCountering();
}

bool PlayerLogic::canChangeElevation() const
{
    return canControlMovement() && !isGuarding();
}

bool PlayerLogic::jump()
{
    return canChangeElevation() && StagePhysics::jump(*m_f);
}

void PlayerLogic::fastFall()
{
    if (!canChangeElevation()) return;
    m_glideTicks = 0;
    StagePhysics::fastFall(*m_f);
}

bool PlayerLogic::canBlink() const
{
    return canChangeElevation() && !isCommittedToAttack();
}

void PlayerLogic::startBlinkInvulnerability()
{
    // update() runs before collision checks on the following simulation tick.
    m_blinkInvulnerabilityTicks = BlinkInvulnerabilityTicks + 1;
}

void PlayerLogic::stepMovement(const Stage &stage)
{
    if (!m_f) return;

    const double beforeX = m_f->x;
    const bool lunging = isLunging() && !isMovementStunned() && lungeVelocity() != 0;
    StagePhysics::step(*m_f, stage, m_glideTicks > 0 ? WindFallSpeed : 0);
    if (m_f->onGround) m_glideTicks = 0;
    m_lungeMoved = lunging && m_f->x != beforeX;
}

bool PlayerLogic::isInvulnerable() const
{
    return m_blinkInvulnerabilityTicks > 0 || (isLunging() && m_lungeMoved);
}

bool PlayerLogic::canUseSkill() const
{
    return m_skill != SkillKind::None && m_skillCooldown == 0 && m_f &&
           (m_f->onGround || m_skill == SkillKind::Wind)
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
    if (m_skill == SkillKind::Wind) {
        const auto spec = windNormalSpec(attackState == AnimState::Attack2);
        m_attackTicks = m_attackCooldown = spec.duration;
        m_windAirNormal = !m_f->onGround;
    }
    if (m_skill == SkillKind::Shadow) {
        m_attackTicks = 26;
        m_attackCooldown = 30;
        m_ninjaShot = NinjaShot::Kunai;
        m_ninjaReleaseAt = m_attackTicks - NinjaThrowWindup;
    }
    m_attackConnected = false;
    m_f->setState(attackState);
    return true;
}

NinjaShot PlayerLogic::takeNinjaShot()
{
    if (m_attackTicks <= 0 || m_attackTicks != m_ninjaReleaseAt) return NinjaShot::None;
    const NinjaShot shot = m_ninjaShot;
    m_ninjaShot = NinjaShot::None;
    return shot;
}

bool PlayerLogic::lunge(int direction)
{
    if (!canBlink()) return false;

    m_glideTicks = 0;
    const LungeSpec spec = lungeSpec(m_skill);
    m_attackTicks = spec.duration;
    m_attackCooldown = spec.duration;
    m_lungeMoved = false;
    m_attackConnected = false;
    m_f->setFacing(direction > 0);
    if (m_skill == SkillKind::Wind && m_f->onGround) StagePhysics::lift(*m_f, 5.8);
    if (m_skill == SkillKind::Shadow) {
        m_ninjaShot = NinjaShot::Shuriken;
        m_ninjaReleaseAt = spec.duration - spec.windup;
    }
    m_f->setState(AnimState::Lunge);
    return true;
}

bool PlayerLogic::counter()
{
    if (m_counterReadyTicks <= 0 || !m_f || m_f->hp <= 0 || m_hurtTicks > 0 ||
        isCasting() || isWindCharging() || isLunging() || isCountering() || isCommittedToAttack()) return false;

    m_glideTicks = 0;
    const auto spec = counterSpec(m_skill);
    clearCounterOpportunity();
    m_blockTicks = 0;
    m_attackTicks = m_attackCooldown = spec.duration;
    m_attackConnected = false;
    m_counterTrace = {};
    if (m_skill == SkillKind::Shadow) {
        m_ninjaShot = NinjaShot::Counter;
        m_ninjaReleaseAt = spec.duration - spec.windup - 1;
    }
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

double PlayerLogic::counterVelocity() const
{
    if (!isCountering() || m_attackConnected) return 0;
    if (m_skill == SkillKind::Shadow)
        return counterTick() < 4 ? (m_f->facingRight ? -5.0 : 5.0) : 0;
    if (m_skill != SkillKind::Lightning) return 0;

    const int activeTick = counterTick() - counterSpec(m_skill).windup;
    if (activeTick < 0 || activeTick >= KnightCounterStepTicks) return 0;

    return (m_f->facingRight ? 1.0 : -1.0) * KnightCounterStepSpeed;
}

void PlayerLogic::stepCounter(const Fighter &target, const Stage &stage)
{
    if (!isCountering() || m_skill != SkillKind::SwordWave ||
        counterTick() != counterSpec(m_skill).windup || target.hp <= 0) return;

    const QPoint start = m_f->getBodyBox().center();
    if (StagePhysics::stepBehind(*m_f, target, stage))
        m_counterTrace = QLine(start, m_f->getBodyBox().center());
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
    const int motion = m_skill == SkillKind::Shadow ? -1 : 1;
    return motion * (m_f->facingRight ? 1 : -1) * double(spec.distance) / spec.travel;
}

bool PlayerLogic::useSkill(bool enhanced, int direction)
{
    if (enhanced) {
        if (m_skill == SkillKind::None || m_skillCooldown != 0 || !canAttack()) return false;
    } else if (!canUseSkill()) {
        return false;
    }

    m_attackTicks = 0;
    m_enhancedCast = enhanced;
    m_skillCooldown = skillSpec(m_skill, enhanced).cooldown;
    if (enhanced && direction != 0) m_f->setFacing(direction > 0);
    if (m_skill == SkillKind::Wind) {
        m_glideTicks = WindGlideTicks;
        if (enhanced || m_f->onGround)
            StagePhysics::lift(*m_f, enhanced ? 11.0 : 8.8);
        else
            m_f->vy = qMin(m_f->vy, WindFallSpeed);
        m_f->setState(AnimState::Jump);
        return true;
    }
    m_castTick = 1;
    m_f->setState(AnimState::Skill);
    return true;
}

bool PlayerLogic::beginWindInput(bool enhanced, int direction)
{
    if (m_skill != SkillKind::Wind || m_windHoldTicks >= 0 || !canUseSkill()) return false;
    m_windHoldTicks = 0;
    m_windInputEnhanced = enhanced;
    m_windInputDirection = direction;
    return true;
}

void PlayerLogic::updateWindInput()
{
    if (m_windHoldTicks < 0) return;
    if (isWindChoosing() && !canUseSkill()) {
        cancelWindInput();
        return;
    }
    if (++m_windHoldTicks == WindHoldThreshold) {
        m_attackTicks = 0;
        m_enhancedCast = m_windInputEnhanced;
        m_windBlinkCost = m_enhancedCast;
        m_glideTicks = 0;
        m_skillCooldown = skillSpec(SkillKind::Wind).cooldown;
        if (m_enhancedCast) m_f->setFacing(m_windInputDirection > 0);
        m_f->vx = 0;
        m_f->setState(AnimState::Skill);
    }
    if (m_windHoldTicks == WindMaxHoldTicks) releaseWindGust();
}

void PlayerLogic::releaseWindInput()
{
    if (isWindCharging()) {
        releaseWindGust();
    } else if (isWindChoosing()) {
        m_windHoldTicks = -1;
        if (useSkill(m_windInputEnhanced, m_windInputDirection))
            m_windBlinkCost = m_windInputEnhanced;
    }
}

void PlayerLogic::releaseWindGust()
{
    const WindGust gust = m_enhancedCast ? WindGust::Pull :
                          m_windCaught ? WindGust::ChargedPush : WindGust::Push;
    m_pendingWindGust = SkillEffect::spawnWind(*m_f, gust);
    m_windHoldTicks = -1;
    m_windCaught = false;
    updateStance();
}

void PlayerLogic::cancelWindInput()
{
    if (m_windHoldTicks >= 0) {
        m_windHoldTicks = -1;
        m_windCaught = false;
        updateStance();
    }
    m_pendingWindGust = {};
}

QRect PlayerLogic::windCatchBox() const
{
    if (!isWindCharging() || m_enhancedCast || m_windCaught ||
        m_windHoldTicks >= WindHoldThreshold + WindCatchTicks) return {};
    const QRect body = m_f->getBodyBox();
    const int left = m_f->facingRight ? body.center().x() - 3 : body.center().x() - 43;
    return QRect(left, body.top() - 6, 47, body.height() + 12);
}

bool PlayerLogic::catchProjectile(const QRect &projectile, int direction)
{
    if (!windCatchBox().intersects(projectile) || direction != (m_f->facingRight ? -1 : 1))
        return false;
    m_windCaught = true;
    return true;
}

bool PlayerLogic::takeWindBlinkCost()
{
    return std::exchange(m_windBlinkCost, false);
}

SkillEffect PlayerLogic::takeWindGust()
{
    return std::exchange(m_pendingWindGust, {});
}

int PlayerLogic::windNormalTick() const
{
    if (m_skill != SkillKind::Wind || !m_f || !isPlayingAttack()) return -1;
    const AnimState state = m_f->state();
    if (state != AnimState::Attack && state != AnimState::Attack2) return -1;
    return windNormalSpec(state == AnimState::Attack2).duration - m_attackTicks;
}

bool PlayerLogic::isWindAirNormal() const
{
    return m_windAirNormal && windNormalTick() >= 0;
}

int PlayerLogic::windLungeReach() const
{
    if (m_skill != SkillKind::Wind || !isLunging()) return 0;
    const auto spec = lungeSpec(m_skill);
    return lungeTick() > spec.windup + spec.travel ? spec.reach : 0;
}

QRect PlayerLogic::windStrikeBox(int reach, bool downward) const
{
    const QRect body = m_f->getBodyBox();
    const int left = m_f->facingRight ? body.center().x() + 5 : body.center().x() - reach - 4;
    const int top = downward ? body.center().y() : body.top() + 9;
    const int bottom = body.bottom() + (downward ? 34 : -7);
    return QRect(left, top, reach, bottom - top + 1);
}

QRect PlayerLogic::nextNormalAttackBox() const
{
    if (!m_f) return {};
    if (m_skill == SkillKind::Wind)
        return windStrikeBox(windNormalSpec(nextAttackState() == AnimState::Attack2).reach, !m_f->onGround);
    return m_f->attackBox(nextAttackState());
}

QRect PlayerLogic::activeAttackBox() const
{
    if (!m_f || m_skill == SkillKind::Shadow || !isCommittedToAttack()) return {};
    int reach = 0;
    if (isLunging()) {
        const auto spec = lungeSpec(m_skill);
        const int tick = lungeTick();
        if (m_skill == SkillKind::Wind) {
            const int strikeStart = spec.windup + spec.travel;
            if (tick <= strikeStart || tick > strikeStart + WindLungeStrikeTicks) return {};
            return windStrikeBox(spec.reach, true);
        }
        if (tick <= spec.windup || tick > spec.windup + spec.travel) return {};
        reach = spec.reach;
    } else if (isCountering()) {
        const auto spec = counterSpec(m_skill);
        const int tick = counterTick();
        if (tick <= spec.windup || tick > spec.windup + spec.active) return {};
        if (m_skill == SkillKind::Wind) {
            const QRect body = m_f->getBodyBox();
            const int left = m_f->facingRight ? body.center().x() + 2 : body.center().x() - spec.reach - 1;
            return QRect(left, body.top() - 44, spec.reach, 44 + body.height() * 3 / 4);
        }
        reach = spec.reach;
    } else if (m_skill == SkillKind::Wind) {
        const auto spec = windNormalSpec(m_f->state() == AnimState::Attack2);
        const int tick = spec.duration - m_attackTicks;
        if (tick <= spec.startup || tick > spec.startup + spec.active) return {};
        return windStrikeBox(spec.reach, m_windAirNormal);
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
    if (activeAttackBox().isEmpty() || !target.m_f || target.m_f->hp <= 0 ||
        target.isInvulnerable()) return {};

    m_attackConnected = true;
    if (m_skill == SkillKind::Wind && !isLunging() && !isCountering()) {
        const auto spec = windNormalSpec(m_f->state() == AnimState::Attack2);
        return target.receiveHit(spec.damage, spec.guardDamage, m_f->getBodyBox().center().x(), spec.knockback);
    }
    const int damage = isCountering() ? counterSpec(m_skill).damage
                                      : isLunging() ? lungeSpec(m_skill).damage : AttackDamage;
    const int guardDamage = isCountering() ? counterSpec(m_skill).guardDamage
                                           : isLunging() ? lungeSpec(m_skill).guardDamage
                                                         : AttackGuardDamage;
    const double knockback = isCountering() ? counterSpec(m_skill).knockback
                                            : isLunging() ? lungeSpec(m_skill).knockback
                                                          : AttackKnockback;
    const int sourceX = m_f->getBodyBox().center().x();
    const bool canLaunch = m_skill == SkillKind::Wind && isCountering() && !target.hasCounterArmor() &&
                           qAbs(sourceX - target.m_f->getBodyBox().center().x()) <= 55;
    const HitResult result = target.receiveHit(damage, guardDamage, sourceX, knockback);
    if (canLaunch && result.damage > 0 && !result.blocked && target.m_f->hp > 0)
        StagePhysics::lift(*target.m_f, 7.3);
    return result;
}

HitResult PlayerLogic::receiveHit(int damage, int guardDamage, int sourceX, double knockback, bool pull)
{
    if (!m_f || m_f->hp <= 0 || damage <= 0 || isInvulnerable()) return {};
    const int previousHp = m_f->hp;
    const int bodyX = m_f->getBodyBox().center().x();
    const int offset = sourceX - bodyX;
    const bool fromFront = m_f->facingRight ? offset > 0 : offset < 0;
    // 普通击退背离攻击来源，牵引则朝向来源。
    const int away = sourceX >= bodyX ? -1 : 1;
    const int knockDir = pull ? -away : away;
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
            applyKnockback(knockDir, pull ? 3.2 : knockSpeed + 1.4);   // 破防比普通受击弹得更远
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
    if (!isMovementStunned()) return false;
    if (m_knockTicks > 0) {
        fighter.vx *= KnockDecay;
        if (qAbs(fighter.vx) < 0.6) fighter.vx = 0;
    } else {
        fighter.vx = 0;
    }
    return true;
}

void PlayerLogic::applyDamage(int damage, int knockDir, double knockSpeed)
{
    m_f->hp = qMax(0, m_f->hp - damage);
    m_f->flashTicks = Fighter::HitFlashTicks;
    m_f->blockFlashTicks = 0;
    m_hurtTicks = HurtTicks;
    cancelWindInput();
    clearCounterOpportunity();
    m_ninjaShot = NinjaShot::None;
    m_glideTicks = 0;
    m_attackTicks = m_blockTicks = m_castTick = 0;
    m_f->setState(AnimState::Hurt);
    applyKnockback(knockDir, knockSpeed);
}

void PlayerLogic::guardBreak()
{
    m_guard = 0;
    m_guardBrokenTicks = GuardBreakTicks;
    cancelWindInput();
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
