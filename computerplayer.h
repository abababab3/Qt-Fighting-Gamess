#ifndef COMPUTERPLAYER_H
#define COMPUTERPLAYER_H

#include "hero.h"
#include "ninja.h"
#include "playerlogic.h"

class ComputerPlayer {
public:
    struct Movement {
        int direction = 0;
        bool jump = false;
        bool fastFall = false;
    };

    // 一套「性格」参数。决策骨架对所有角色是同一套（找距离 → 判断能不能打 → 打），
    // 但不同角色的招式表、破盾能力、反击范围差得很远，靠这几个旋钮分流：
    //   Martial Hero：沿用改造前的数值，保证 AI 手感不漂移
    //   Hero Knight ：爱举盾、爱反击、对手一举盾就丢落雷拆盾、抓住硬直立刻惩罚
    struct Personality {
        int reactionMin = 12;      // 出招间隔（tick）下限
        int reactionMax = 23;      // 出招间隔（tick）上限
        int guardChance = 40;      // 察觉对手起手后举盾的概率（%）
        int guardTicks = 26;       // 一次举盾维持帧数
        int reactRange = 145;      // 多近才开始考虑防御反应
        int counterRange = 84;     // 触发反击的距离
        int counterDelay = 6;      // 完美格挡后等多少帧再出手（越小越急）
        int jumpRange = 167;       // 越过高低差时的起跳距离
        int skillDivisor = 3;      // 普攻 / 技能二选一的分母，越大越少放技能
        int guardBreakChance = 0;  // 对手举盾时改用技能拆盾的概率（%）
        bool punishHurt = false;   // 对手进入受击硬直时是否立刻抢招
    };

    void setHero(HeroKind hero);
    void reset();
    Movement update(Fighter &fighter, const Fighter &opponent, PlayerLogic &logic,
                    NinjaKit *ninja = nullptr, const Stage *stage = nullptr);

private:
    int reactionDelay() const;

    static Personality personalityFor(HeroKind hero);

    HeroKind m_hero = HeroKind::MartialHero;
    Personality m_traits = personalityFor(HeroKind::MartialHero);
    int m_attackWait = 0;
    int m_ticksInRange = 0;
    int m_jumpCooldown = 0;
    int m_guardTicks = 0;
    bool m_sawAttack = false;
};

#endif // COMPUTERPLAYER_H
