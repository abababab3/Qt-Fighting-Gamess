#ifndef COMPUTERPLAYER_H
#define COMPUTERPLAYER_H

#include "playerlogic.h"

class ComputerPlayer {
public:
    struct Movement {
        int direction = 0;
        bool jump = false;
        bool fastFall = false;
    };

    void reset();
    Movement update(Fighter &fighter, const Fighter &opponent, PlayerLogic &logic);

private:
    int m_attackWait = 0;
    int m_ticksInRange = 0;
    int m_jumpCooldown = 0;
    int m_guardTicks = 0;
    bool m_sawAttack = false;
};

#endif // COMPUTERPLAYER_H
