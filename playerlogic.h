#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

#include "fighter.h"

class PlayerLogic {
public:
    explicit PlayerLogic(Fighter *f = nullptr) : m_f(f) {}
    void attach(Fighter *f) { m_f = f; }

    void update();                              // 每帧推进
    bool attack();                              // 发起攻击
    void checkHit(bool hit, Fighter *target);   // 命中则扣血
    void takeDamage(int dmg);
    void takeDamageTo(Fighter *target, int dmg);

    bool canAttack() const { return m_attackCooldown <= 0; }

private:
    Fighter *m_f = nullptr;
    int m_attackCooldown = 0;
    int m_attackAnimTimer = 0;
    int m_hurtTimer = 0;
    bool m_attacking = false;

    static constexpr int ATTACK_COOLDOWN = 28;
    static constexpr int ATTACK_ANIM     = 18;
    static constexpr int ATTACK_DAMAGE   = 8;
    static constexpr int HURT_TIME       = 12;
};

#endif // PLAYERLOGIC_H