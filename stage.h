#ifndef STAGE_H
#define STAGE_H

#include <QList>
#include <QRect>
#include <QString>

class Fighter;

enum class StageId { Moonlit, Bamboo, Canyon, Count };

struct Stage {
    StageId id;
    QString name;
    QList<QRect> platforms;

    static constexpr int Width = 800;
    static constexpr int GroundY = 500;
};

const Stage &stageFor(StageId id);

namespace StagePhysics {
void step(Fighter &fighter, const Stage &stage, double fallSpeedLimit = 0);
bool jump(Fighter &fighter);
void lift(Fighter &fighter, double speed);
bool blink(Fighter &fighter, int distance, const Stage &stage);
bool stepBehind(Fighter &fighter, const Fighter &target, const Stage &stage);
void relocate(Fighter &fighter, const QRect &destination, const Stage &stage);
void fastFall(Fighter &fighter);
int surfaceBelow(const QRect &body, const Stage &stage);
}

#endif // STAGE_H
