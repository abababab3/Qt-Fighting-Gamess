#include "computerplayer.h"

#include <QRandomGenerator>

namespace {

int reactionDelay()
{
    return QRandomGenerator::global()->bounded(12, 23);
}

} // namespace

void ComputerPlayer::reset()
{
    m_attackWait = reactionDelay();
    m_ticksInRange = 0;
    m_jumpCooldown = 0;
    m_guardTicks = 0;
    m_sawAttack = false;
}

ComputerPlayer::Movement ComputerPlayer::update(Fighter &fighter, const Fighter &opponent,
                                                PlayerLogic &logic)
{
    if (m_jumpCooldown > 0) --m_jumpCooldown;
    if (m_guardTicks > 0) --m_guardTicks;
    logic.setGuardHeld(m_guardTicks > 0);

    const AnimState state = fighter.state();
    if (logic.isCommittedToAttack() || state == AnimState::Hurt || logic.isCasting() || logic.isCountering()) {
        m_ticksInRange = 0;
        return {};
    }

    const QRect target = opponent.getBodyBox();
    const int offset = target.center().x() - fighter.getBodyBox().center().x();
    const bool attacking = opponent.state() == AnimState::Attack || opponent.state() == AnimState::Attack2
                           || opponent.state() == AnimState::Skill || opponent.state() == AnimState::Lunge
                           || opponent.state() == AnimState::Counter;
    if (!attacking) m_sawAttack = false;
    // 电脑没有方向键，它的"转身操作"就是这一行，所以举盾时也照常面向玩家。
    // 玩家侧相反：举盾锁朝向，要自己按方向键转身——这是玩家该付的操作成本；
    // 而 AI 若也不转身，绕到背后就能白嫖它，难度只会更低。
    if (offset != 0) fighter.setFacing(offset > 0);

    if (!m_sawAttack && attacking && qAbs(offset) < 145 && logic.canGuard()) {
        m_sawAttack = true;
        if (QRandomGenerator::global()->bounded(100) < 40) {
            m_guardTicks = 26;
            logic.setGuardHeld(true);
        }
    }
    const bool targetInFront = fighter.facingRight ? offset > 0 : offset < 0;
    if (logic.counterReadyTicks() <= PlayerLogic::CounterReadyTicks - 6 && targetInFront &&
        qAbs(offset) <= 84 && qAbs(target.center().y() - fighter.getBodyBox().center().y()) < 40 &&
        logic.counter()) {
        m_ticksInRange = 0;
        return {};
    }
    if (logic.isGuarding()) {
        m_ticksInRange = 0;
        return {};
    }

    const QRect attackBox = fighter.attackBox(logic.nextAttackState());
    const bool normalInRange = attackBox.intersects(target);
    const bool skillInRange = logic.canUseSkill()
                              && SkillEffect::spawn(logic.skill(), fighter).area.intersects(target);
    if (normalInRange || skillInRange) {
        if (!logic.canAttack()) {
            m_ticksInRange = 0;
        } else if (++m_ticksInRange >= m_attackWait) {
            const bool chooseSkill = skillInRange
                                     && (!normalInRange || QRandomGenerator::global()->bounded(3) == 0);
            if (chooseSkill) logic.useSkill();
            else logic.attack();
            m_ticksInRange = 0;
            m_attackWait = reactionDelay();
        }
        return {};
    }

    m_ticksInRange = 0;
    const int approach = target.right() < attackBox.left() ? -1
                       : target.left() > attackBox.right() ? 1 : 0;
    const int elevation = fighter.getBodyBox().bottom() - target.bottom();
    if (fighter.onGround && elevation > 26 && qAbs(offset) < 167 && m_jumpCooldown == 0) {
        m_jumpCooldown = 38;
        return {approach, true};
    }
    if (fighter.standingPlatform >= 0 && elevation < -32 && qAbs(offset) < 155)
        return {approach, false, true};
    if (approach != 0) return {approach, false};
    return {};
}
