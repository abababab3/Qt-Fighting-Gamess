#include "gameui.h"
#include <QFont>

GameUI::GameUI() {}

void GameUI::drawHealthBars(QPainter &p) {
    const int barW = 300, barH = 24, y = 30, margin = 30;
    int winW = 800;

    QRect bg1(margin, y, barW, barH);
    p.fillRect(bg1, QColor(60,60,60));
    p.fillRect(QRect(margin, y, barW * p1Health / m_maxHealth, barH), QColor(220,50,50));
    p.setPen(Qt::white); p.drawRect(bg1);

    int x2 = winW - margin - barW;
    QRect bg2(x2, y, barW, barH);
    p.fillRect(bg2, QColor(60,60,60));
    p.fillRect(QRect(x2 + barW - barW*p2Health/m_maxHealth, y,
                     barW*p2Health/m_maxHealth, barH), QColor(50,120,220));
    p.setPen(Qt::white); p.drawRect(bg2);
}

void GameUI::drawKOText(QPainter &p, const QString &msg) {
    QFont f = p.font(); f.setPointSize(48); f.setBold(true);
    p.setFont(f); p.setPen(Qt::yellow);
    p.drawText(QRect(0, 0, 800, 600), Qt::AlignCenter, msg);
}