#include "computerplayer.h"

#include <QRandomGenerator>

ComputerPlayer::Personality ComputerPlayer::personalityFor(HeroKind hero)
{
    if (hero == HeroKind::HeroKnight) {
        Personality knight;
        // 反应更快，但更"稳"：优先用盾牌换机会，再用落雷拆盾、用反击惩罚冒进
        knight.reactionMin = 10;
        knight.reactionMax = 19;
        knight.guardChance = 62;
        knight.guardTicks = 34;
        knight.reactRange = 150;
        knight.counterRange = 96;        // 骑士反击 reach 70，比剑客长一截
        knight.counterDelay = 3;
        knight.jumpRange = 150;
        knight.skillDivisor = 2;         // 落雷是核心拆盾手段，出得更勤
        knight.guardBreakChance = 80;    // 对手一举盾就砸落雷（56 破盾，两发拆满）
        knight.punishHurt = true;        // 盾碎 / 受击硬直期间立刻抢招
        return knight;
    }
    // Martial Hero：以下数值与改造前完全一致
    return Personality{};
}

void ComputerPlayer::setHero(HeroKind hero)
{
    if (m_hero == hero) return;
    m_hero = hero;
    m_traits = personalityFor(hero);
    reset();
}

int ComputerPlayer::reactionDelay() const
{
    return QRandomGenerator::global()->bounded(m_traits.reactionMin, m_traits.reactionMax);
}

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

    // ---- 防御反应：看到起手就举盾（骑士更愿意防，也防得更久）----
    if (!m_sawAttack && attacking && qAbs(offset) < m_traits.reactRange && logic.canGuard()) {
        m_sawAttack = true;
        if (QRandomGenerator::global()->bounded(100) < m_traits.guardChance) {
            m_guardTicks = m_traits.guardTicks;
            logic.setGuardHeld(true);
        }
    }
    const bool targetInFront = fighter.facingRight ? offset > 0 : offset < 0;
    if (logic.counterReadyTicks() <= PlayerLogic::CounterReadyTicks - m_traits.counterDelay &&
        targetInFront &&
        qAbs(offset) <= m_traits.counterRange && qAbs(target.center().y() - fighter.getBodyBox().center().y()) < 40 &&
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
        // ---- 拆盾：对手举盾时，技能的高破盾值比平A划算得多 ----
        // 剑客的剑气破盾 34 只是聊胜于无，骑士的落雷是 56（两发拆满），所以这条只给骑士开
        if (opponent.state() == AnimState::Guard && skillInRange && m_traits.guardBreakChance > 0
            && logic.canUseSkill()
            && QRandomGenerator::global()->bounded(100) < m_traits.guardBreakChance) {
            logic.useSkill();
            m_ticksInRange = 0;
            m_attackWait = reactionDelay();
            return {};
        }

        if (!logic.canAttack()) {
            m_ticksInRange = 0;
        } else {
            // 对手正在受击硬直里：骑士会跳过反应延迟直接抢招，把硬直吃满
            const int wait = (m_traits.punishHurt && opponent.state() == AnimState::Hurt) ? 2 : m_attackWait;
            if (++m_ticksInRange >= wait) {
                const bool chooseSkill = skillInRange
                                         && (!normalInRange
                                             || QRandomGenerator::global()->bounded(m_traits.skillDivisor) == 0);
                if (chooseSkill) logic.useSkill();
                else logic.attack();
                m_ticksInRange = 0;
                m_attackWait = reactionDelay();
            }
        }
        return {};
    }

    m_ticksInRange = 0;
    const int approach = target.right() < attackBox.left() ? -1
                       : target.left() > attackBox.right() ? 1 : 0;
    const int elevation = fighter.getBodyBox().bottom() - target.bottom();
    if (fighter.onGround && elevation > 26 && qAbs(offset) < m_traits.jumpRange && m_jumpCooldown == 0) {
        m_jumpCooldown = 38;
        return {approach, true};
    }
    if (fighter.standingPlatform >= 0 && elevation < -32 && qAbs(offset) < m_traits.jumpRange - 12)
        return {approach, false, true};
    if (approach != 0) return {approach, false};
    return {};
}
