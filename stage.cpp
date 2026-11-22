#include "stage.h"

#include "fighter.h"
#include <array>

namespace {

bool overlapsHorizontally(const QRect &body, const QRect &platform)
{
    return body.left() <= platform.right() && body.right() >= platform.left();
}

double boundedX(const Fighter &fighter, double x)
{
    return qBound(0.0, x, static_cast<double>(Stage::Width - fighter.getFrameWidth()));
}

constexpr double Gravity = 0.9;
constexpr double JumpVelocity = -16.0;
constexpr double FastFallSpeed = 18.0;

} // namespace

const Stage &stageFor(StageId id)
{
    static const std::array<Stage, static_cast<int>(StageId::Count)> stages{{
        {StageId::Moonlit, QStringLiteral("月夜擂台"), {}},
        {StageId::Bamboo, QStringLiteral("竹影栈台"),
         {QRect(103, 417, 170, 12), QRect(318, 381, 164, 12), QRect(527, 417, 170, 12)}},
        {StageId::Canyon, QStringLiteral("赤岩峡谷"),
         {QRect(125, 397, 220, 15), QRect(455, 428, 220, 15)}}
    }};
    return stages[static_cast<std::size_t>(id)];
}

void StagePhysics::step(Fighter &fighter, const Stage &stage, double fallSpeedLimit)
{
    const int previousBottom = fighter.getBodyBox().bottom();
    fighter.vy += Gravity;
    if (fallSpeedLimit > 0) fighter.vy = qMin(fighter.vy, fallSpeedLimit);
    fighter.x = boundedX(fighter, fighter.x + fighter.vx);
    fighter.y += fighter.vy;

    const QRect body = fighter.getBodyBox();
    int landingY = Stage::GroundY;
    int landingPlatform = -1;
    if (fighter.vy >= 0) {
        for (int i = 0; i < stage.platforms.size(); ++i) {
            const QRect &platform = stage.platforms[i];
            if (i != fighter.ignoredPlatform && platform.top() < landingY &&
                previousBottom <= platform.top() && body.bottom() >= platform.top() &&
                overlapsHorizontally(body, platform)) {
                landingY = platform.top();
                landingPlatform = i;
            }
        }
    }

    fighter.onGround = body.bottom() >= landingY;
    fighter.standingPlatform = fighter.onGround ? landingPlatform : -1;
    if (fighter.onGround) {
        fighter.y += landingY - body.bottom();
        fighter.vy = 0;
    }

    // Once the feet pass through the old surface it is behind the next crossing test.
    if (fighter.ignoredPlatform >= 0 &&
        fighter.getBodyBox().bottom() > stage.platforms[fighter.ignoredPlatform].top()) {
        fighter.ignoredPlatform = -1;
    }
}

bool StagePhysics::blink(Fighter &fighter, int distance, const Stage &stage)
{
    const double destination = boundedX(fighter, fighter.x + distance);
    if (qAbs(destination - fighter.x) < 1.0) return false;

    fighter.x = destination;
    fighter.setFacing(distance > 0);
    if (fighter.standingPlatform >= 0) {
        const QRect body = fighter.getBodyBox();
        fighter.standingPlatform = -1;
        for (int i = 0; i < stage.platforms.size(); ++i) {
            const QRect &platform = stage.platforms[i];
            if (platform.top() == body.bottom() && overlapsHorizontally(body, platform)) {
                fighter.standingPlatform = i;
                break;
            }
        }
        fighter.onGround = fighter.standingPlatform >= 0;
    }
    return true;
}

bool StagePhysics::stepBehind(Fighter &fighter, const Fighter &target, const Stage &stage)
{
    constexpr int RearGap = 6;
    const QRect body = fighter.getBodyBox();
    const QRect targetBody = target.getBodyBox();
    const bool behindOnLeft = target.facingRight;
    const int shift = behindOnLeft ? targetBody.left() - RearGap - body.right()
                                  : targetBody.right() + RearGap - body.left();
    const double destination = fighter.x + shift;
    if (qAbs(boundedX(fighter, destination) - destination) > 0.01) {
        const int offset = targetBody.center().x() - body.center().x();
        if (offset != 0) fighter.setFacing(offset > 0);
        return false;
    }

    relocate(fighter, body.translated(shift, targetBody.bottom() - body.bottom()), stage);
    fighter.setFacing(behindOnLeft);
    return true;
}

void StagePhysics::relocate(Fighter &fighter, const QRect &destination, const Stage &stage)
{
    const QRect body = fighter.getBodyBox();
    fighter.x = boundedX(fighter, fighter.x + destination.center().x() - body.center().x());
    fighter.y += qMin(destination.bottom(), Stage::GroundY) - body.bottom();
    fighter.vx = fighter.vy = 0;
    fighter.standingPlatform = -1;
    fighter.ignoredPlatform = -1;

    const QRect landing = fighter.getBodyBox();
    for (int i = 0; i < stage.platforms.size(); ++i) {
        const auto &platform = stage.platforms.at(i);
        if (platform.top() == landing.bottom() && overlapsHorizontally(landing, platform)) {
            fighter.standingPlatform = i;
            break;
        }
    }
    fighter.onGround = landing.bottom() >= Stage::GroundY || fighter.standingPlatform >= 0;
}

bool StagePhysics::jump(Fighter &fighter)
{
    if (!fighter.onGround) return false;
    lift(fighter, -JumpVelocity);
    return true;
}

void StagePhysics::lift(Fighter &fighter, double speed)
{
    fighter.vy = -speed;
    fighter.onGround = false;
    fighter.standingPlatform = -1;
}

void StagePhysics::fastFall(Fighter &fighter)
{
    if (fighter.onGround && fighter.standingPlatform < 0) return;

    if (fighter.standingPlatform >= 0) fighter.ignoredPlatform = fighter.standingPlatform;
    fighter.standingPlatform = -1;
    fighter.onGround = false;
    fighter.vy = qMax(fighter.vy, FastFallSpeed);
}

int StagePhysics::surfaceBelow(const QRect &body, const Stage &stage)
{
    int surface = Stage::GroundY;
    for (const QRect &platform : stage.platforms) {
        if (platform.top() >= body.bottom() && overlapsHorizontally(body, platform))
            surface = qMin(surface, platform.top());
    }
    return surface;
}
