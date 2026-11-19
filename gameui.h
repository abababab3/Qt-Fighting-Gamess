#ifndef GAMEUI_H
#define GAMEUI_H

#include <QPainter>

// 粗糙版：只当"倒计时数据源"。血条/KO 走 .ui 控件（组员B 在 Designer 美化）。
class GameUI {
public:
    GameUI();

    void resetTimer(int sec) { m_timeLeft = sec; }
    int  getTimeLeft() const { return m_timeLeft; }
    void tickTimer() { if (m_timeLeft > 0) --m_timeLeft; }

    // 备用：如果你想纯代码画，用这些（组员B 可自行扩展）
    void drawHealthBars(QPainter &p);
    void drawKOText(QPainter &p, const QString &msg);

    void setMaxHealth(int m) { m_maxHealth = m; }
    void updateHealth(int p1, int p2) { p1Health = p1; p2Health = p2; }

private:
    int p1Health = 100, p2Health = 100, m_maxHealth = 100;
    int m_timeLeft = 60;
};

#endif // GAMEUI_H