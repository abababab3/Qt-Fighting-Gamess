#ifndef NINJA_H
#define NINJA_H

#include "combatskill.h"

class Fighter;
struct Stage;

struct NinjaProjectile {
    NinjaShot kind = NinjaShot::None;
    QRect area;
    int direction = 0;
    int ticks = 0;

    static NinjaProjectile launch(NinjaShot kind, const QRect &body, int direction);
    void advance();
    QRect hitBox() const { return ticks > 0 ? area : QRect(); }
};

class NinjaKit {
public:
    static constexpr int CloneTicks = 150;
    static constexpr int EchoDelay = 11;
    static constexpr int SmokeTicks = 12;

    void planClone(const QRect &body) { m_plannedClone = body; }
    void placeClone(const Fighter &fighter);
    bool hasClone() const { return m_cloneTicks > 0; }
    bool echoPending() const { return m_echoTicks > 0; }
    int cloneTicks() const { return m_cloneTicks; }
    int smokeTicks() const { return m_smokeTicks; }
    QRect cloneBox() const { return hasClone() ? m_clone : QRect(); }
    QRect smokeFrom() const { return m_smokeFrom; }
    QRect smokeTo() const { return m_smokeTo; }
    bool facesRight() const { return m_cloneDirection > 0; }
    bool hasVisuals() const { return hasClone() || m_smokeTicks > 0 || own.ticks > 0 || echo.ticks > 0; }

    void commitEcho(const Fighter &target, int fallbackDirection);
    bool swap(Fighter &fighter, const Stage &stage);
    void disrupt(const QRect &attack);
    void advance();

    NinjaProjectile own;
    NinjaProjectile echo;

private:
    void fadeClone(const QRect &destination = {});

    QRect m_plannedClone;
    QRect m_clone;
    QRect m_smokeFrom;
    QRect m_smokeTo;
    int m_cloneTicks = 0;
    int m_echoTicks = 0;
    int m_smokeTicks = 0;
    int m_cloneDirection = 1;
};

#endif
