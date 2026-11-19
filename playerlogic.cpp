#include "playerlogic.h"

void PlayerLogic::update() {
    if (m_attackCooldown > 0) --m_attackCooldown;
    if (m_hurtTimer > 0)      --m_hurtTimer;
    // 命中闪白递减由 mainwindow 统一处理，这里只管逻辑状态

    if (m_attacking) {
        if (--m_attackAnimTimer <= 0) {
            m_attacking = false;
            if (m_f && m_f->hp > 0) {
                // 【空中攻击收尾】攻击播完后：还在空中就回 Jump，落地了才回 Idle
                if (!m_f->onGround) m_f->setState(AnimState::Jump);
                else                m_f->setState(AnimState::Idle);
            }
        }
    } else if (m_f && m_hurtTimer == 0) {
        if (!m_f->onGround)      m_f->setState(AnimState::Jump);
        else if (m_f->vx != 0)   m_f->setState(AnimState::Walk);
        else                     m_f->setState(AnimState::Idle);
    }
}

bool PlayerLogic::attack() {
    if (!m_f || !canAttack() || m_attacking) return false;
    m_attacking = true;

    // 每次按键在两组攻击之间交替：attack1 → attack2 → attack1 ...
    AnimState atk = m_useAttack2 ? AnimState::Attack2 : AnimState::Attack;
    m_useAttack2 = !m_useAttack2;

    // 动态时长：按当前这组动画的实际帧数 × 6 tick，多留几 tick 停住末帧。
    int frames = m_f->getFrameCount(atk);
    if (frames <= 0) frames = (atk == AnimState::Attack2) ? 7 : 6;
    m_attackAnimTimer = frames * 6 + 6;
    m_attackCooldown = ATTACK_COOLDOWN;
    m_f->setState(atk);
    return true;
}

void PlayerLogic::checkHit(bool hit, PlayerLogic *targetLogic) {
    if (!hit || !m_attacking || !targetLogic) return;
    if (!targetLogic->m_f || targetLogic->m_f->hp <= 0) return;

    targetLogic->takeDamage(ATTACK_DAMAGE);

    m_attacking = false;
    m_attackAnimTimer = 0;
    if (m_f && m_f->hp > 0) {
        if (!m_f->onGround) m_f->setState(AnimState::Jump);
        else                m_f->setState(AnimState::Idle);
    }
}

void PlayerLogic::takeDamage(int dmg) {
    if (!m_f) return;
    m_f->hp -= dmg;
    if (m_f->hp < 0) m_f->hp = 0;
    m_hurtTimer = HURT_TIME;
    m_f->flashTicks = 10;          // 命中闪白
    m_attacking = false;
    m_attackAnimTimer = 0;
    m_f->setState(AnimState::Hurt);
}

void PlayerLogic::takeDamageTo(Fighter *target, int dmg) {
    if (!target) return;
    target->hp -= dmg;
    if (target->hp < 0) target->hp = 0;
    target->flashTicks = 10;
    target->setState(AnimState::Hurt);
}
