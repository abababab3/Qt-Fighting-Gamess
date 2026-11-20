#ifndef BLINK_H
#define BLINK_H

#include <QPoint>

class Fighter;
struct Stage;

class Blink {
public:
    static constexpr int Distance = 108;
    static constexpr int TapWindowMs = 240;
    static constexpr int CooldownTicks = 80;
    static constexpr int TrailTicks = 10;
    static constexpr int FollowUpTicks = 28;

    bool tap(int direction, qint64 now, bool allowed, Fighter &fighter, const Stage &stage);
    void clearTap();
    void advance();

    int cooldown() const { return m_cooldown; }
    int trailTicks() const { return m_trailTicks; }
    int followUpTicks() const { return m_followUpTicks; }
    int followUpDirection() const { return m_trailEnd.x() > m_trailStart.x() ? 1 : -1; }
    void clearFollowUp() { m_followUpTicks = 0; }
    QPoint trailStart() const { return m_trailStart; }
    QPoint trailEnd() const { return m_trailEnd; }

private:
    int m_tapDirection = 0;
    qint64 m_lastTap = 0;
    int m_cooldown = 0;
    int m_trailTicks = 0;
    int m_followUpTicks = 0;
    QPoint m_trailStart;
    QPoint m_trailEnd;
};

#endif // BLINK_H
