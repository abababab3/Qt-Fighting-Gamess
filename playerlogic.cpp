#include "playerlogic.h"

void PlayerLogic::update() {
    if (m_attackCooldown > 0) --m_attackCooldown;
    if (m_hurtTimer > 0)      --m_hurtTimer;

    if (m_attacking) {
        if (--m_attackAnimTimer <= 0) {
            m_attacking = false;
            if (m_f && m_f->hp > 0) m_f->setState(AnimState::Idle);
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
    m_attackAnimTimer = ATTACK_ANIM;
    m_attackCooldown = ATTACK_COOLDOWN;
    m_f->setState(AnimState::Attack);
    return true;
}

void PlayerLogic::checkHit(bool hit, Fighter *target) {
    if (!hit || !m_attacking || !target || target->hp <= 0) return;
    takeDamageTo(target, ATTACK_DAMAGE);
    m_attacking = false;
    m_attackAnimTimer = 0;
    if (m_f && m_f->hp > 0) m_f->setState(AnimState::Idle);
}

void PlayerLogic::takeDamage(int dmg) {
    if (!m_f) return;
    m_f->hp -= dmg;
    if (m_f->hp < 0) m_f->hp = 0;
    m_hurtTimer = HURT_TIME;
    m_f->setState(AnimState::Hurt);
}

void PlayerLogic::takeDamageTo(Fighter *target, int dmg) {
    target->hp -= dmg;
    if (target->hp < 0) target->hp = 0;
    target->setState(AnimState::Hurt);
}