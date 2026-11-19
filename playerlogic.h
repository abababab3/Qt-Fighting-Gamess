#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

#include "fighter.h"

class PlayerLogic {
public:
    explicit PlayerLogic(Fighter *f = nullptr) : m_f(f) {}
    void attach(Fighter *f) { m_f = f; }

    void update();
    bool attack();
    // 统一签名：命中时传对方逻辑，好让对方进入受击/闪白
    void checkHit(bool hit, PlayerLogic *targetLogic);
    void takeDamage(int dmg);
    void takeDamageTo(Fighter *target, int dmg);

    bool canAttack() const { return m_attackCooldown <= 0 && m_hurtTimer <= 0; }

private:
    Fighter *m_f = nullptr;
    int m_attackCooldown = 0;
    int m_attackAnimTimer = 0;
    int m_hurtTimer = 0;
    bool m_attacking = false;
    bool m_useAttack2 = false;   // 下次攻击用第二组(attack2)，每次 attack() 后取反

    static constexpr int ATTACK_COOLDOWN = 24;
    // 攻击动画时长在 attack() 里按帧数动态算，保证 6~7 帧完整播完
    // （原来写死 18 tick，每 6 tick 换一帧，只播 3 帧，挥拳帧根本不出现）
    static constexpr int ATTACK_DAMAGE   = 8;
    static constexpr int HURT_TIME       = 12;
};

#endif // PLAYERLOGIC_H
