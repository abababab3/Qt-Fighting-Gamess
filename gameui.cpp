#include "gameui.h"
#include "blink.h"
#include "fighter.h"
#include "playerlogic.h"

#include <QFont>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QPolygon>

namespace {

const QColor Ink("#f6ebda");
const QColor Muted("#c0afa9");
const QColor Gold("#f4c584");
const QColor Red("#ffa38e");
const QColor Blue("#a9e0df");
const QColor Wood("#d4ad79");
const QColor Border("#655054");
const QColor HurtboxColor("#6bd7ff");
const QColor HitboxColor("#ff9764");
const QColor SwordColor("#c99fff");
const QColor LightningColor("#ffe28d");

constexpr int Left = Qt::AlignLeft | Qt::AlignVCenter;
constexpr int Right = Qt::AlignRight | Qt::AlignVCenter;

struct ArenaPalette {
    QColor sky, dusk, horizon;
    QColor farRidge, middleRidge, foreground;
    QColor light, edge, floor;
};

ArenaPalette arenaPalette(StageId id)
{
    switch (id) {
    case StageId::Bamboo:
        return {QColor("#121f27"), QColor("#26443e"), QColor("#637569"),
                QColor("#43645e"), QColor("#34534a"), QColor("#203c37"),
                QColor("#dce1b0"), QColor("#afd0a7"), QColor("#365048")};
    case StageId::Canyon:
        return {QColor("#251b2b"), QColor("#5a343c"), QColor("#a75d49"),
                QColor("#784b50"), QColor("#56383f"), QColor("#3c2a36"),
                QColor("#ffc188"), QColor("#e6aa7b"), QColor("#65413e")};
    default:
        return {QColor("#191a2b"), QColor("#333047"), QColor("#87585b"),
                QColor("#4e4053"), QColor("#342c40"), QColor("#292536"),
                QColor("#eec8a3"), QColor("#d4a27e"), QColor("#42333e")};
    }
}

void text(QPainter &p, const QRect &area, const QString &value, int size,
          const QColor &color, bool bold = false, int alignment = Qt::AlignCenter)
{
    QFont font = p.font();
    font.setPixelSize(size);
    font.setBold(bold);
    p.setFont(font);
    p.setPen(color);
    p.drawText(area, alignment, value);
}

void panel(QPainter &p, const QRect &area, const QColor &border = Border)
{
    p.setPen(QPen(border, 1));
    p.setBrush(QColor(26, 23, 34, 246));
    p.drawRoundedRect(area, 10, 10);
}

void keyHint(QPainter &p, int x, int y, int keyWidth, const QString &key, const QString &action)
{
    const QRect cap(x, y, keyWidth, 24);
    p.setPen(QColor("#70585b"));
    p.setBrush(QColor("#302731"));
    p.drawRoundedRect(cap, 4, 4);
    text(p, cap, key, 13, Ink, true);
    text(p, QRect(cap.right() + 7, y, 82, cap.height()), action, 14, Muted, false, Left);
}

void drawFooter(QPainter &p, bool training)
{
    if (training) keyHint(p, 407, 550, 24, QStringLiteral("R"), QStringLiteral("重置练习"));
    keyHint(p, 518, 550, 38, QStringLiteral("F12"), QStringLiteral("判定框"));
    keyHint(p, 628, 550, 38, QStringLiteral("Esc"), QStringLiteral("返回主菜单"));
}

void healthBar(QPainter &p, const QRect &track, const GameUI::HealthBar &health,
               const QColor &accent, bool fromRight)
{
    p.save();
    QPainterPath clip;
    clip.addRoundedRect(track, 4, 4);
    p.setClipPath(clip, Qt::IntersectClip);
    const auto fill = [&](qreal hp, const QBrush &brush) {
        const int width = health.maxHp > 0
                              ? qRound(track.width() * qBound(0.0, hp, qreal(health.maxHp)) / health.maxHp)
                              : 0;
        const int x = fromRight ? track.x() + track.width() - width : track.x();
        p.fillRect(QRect(x, track.y(), width, track.height()), brush);
    };

    p.fillRect(track, QColor("#392e3a"));
    fill(health.trailHp, QBrush(Gold));
    QLinearGradient gradient(track.topLeft(), track.bottomLeft());
    gradient.setColorAt(0, accent.lighter(108));
    gradient.setColorAt(1, accent.darker(124));
    fill(health.hp, QBrush(gradient));
    p.setPen(QColor(30, 20, 28, 104));
    for (int part = 1; part < 4; ++part) {
        const int x = track.x() + part * track.width() / 4;
        p.drawLine(x, track.top(), x, track.bottom());
    }
    p.restore();
}

void drawHealthCards(QPainter &p, const GameUI::HealthBar &p1, const GameUI::HealthBar &p2,
                     const QString &opponentName, const QColor &opponentColor)
{
    panel(p, QRect(25, 22, 311, 78));
    panel(p, QRect(464, 22, 311, 78));
    p.fillRect(QRect(38, 38, 3, 16), Red);
    p.fillRect(QRect(759, 38, 3, 16), opponentColor);

    text(p, QRect(48, 31, 140, 30), QStringLiteral("玩家 1"), 18, Red, true, Left);
    text(p, QRect(204, 29, 118, 34), QString::number(p1.hp), 26, Ink, true, Right);
    text(p, QRect(478, 29, 118, 34), QString::number(p2.hp), 26, Ink, true, Left);
    text(p, QRect(612, 31, 140, 30), opponentName, 18, opponentColor, true, Right);
    healthBar(p, QRect(38, 68, 284, 17), p1, Red, true);
    healthBar(p, QRect(478, 68, 284, 17), p2, opponentColor, false);
}

void debugBox(QPainter &p, const QRect &box, QColor color, Qt::PenStyle style = Qt::SolidLine)
{
    if (!box.isValid()) return;

    p.setPen(QPen(color, 1.5, style));
    color.setAlpha(style == Qt::SolidLine ? 48 : 0);
    p.setBrush(color);
    p.drawRect(QRectF(box));
}

} // namespace

namespace GameUI {

void HealthTrail::reset(int hp)
{
    m_hp = hp;
    m_previous = hp;
    m_hitAt = 0;
}

void HealthTrail::update(int hp, qint64 elapsed)
{
    if (hp == m_hp) return;
    if (hp > m_hp) {
        reset(hp);
        return;
    }

    m_previous = value(elapsed);
    m_hp = hp;
    m_hitAt = elapsed;
}

qreal HealthTrail::value(qint64 elapsed) const
{
    constexpr qreal holdMs = 160;
    constexpr qreal drainMs = 300;
    const qreal progress = qBound(0.0, (elapsed - m_hitAt - holdMs) / drainMs, 1.0);
    return m_previous + (m_hp - m_previous) * progress;
}

void drawArena(QPainter &p, const Stage &stage)
{
    const ArenaPalette palette = arenaPalette(stage.id);
    QLinearGradient sky(0, 0, 0, GroundY);
    sky.setColorAt(0, palette.sky);
    sky.setColorAt(0.56, palette.dusk);
    sky.setColorAt(1, palette.horizon);
    p.fillRect(0, 0, Width, GroundY, sky);

    const QPointF lightCenter(623, 197);
    QRadialGradient glow(lightCenter, 104);
    QColor glowColor = palette.light;
    glowColor.setAlpha(92);
    glow.setColorAt(0, glowColor);
    glowColor.setAlpha(0);
    glow.setColorAt(1, glowColor);
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(lightCenter, 104, 104);
    p.setBrush(palette.light);
    p.drawEllipse(lightCenter, 38, 38);

    p.setBrush(palette.farRidge);
    if (stage.id == StageId::Canyon) {
        p.drawPolygon(QPolygon{{0, 361}, {76, 355}, {112, 300}, {215, 300}, {252, 361},
                               {327, 380}, {381, 316}, {500, 316}, {553, 388},
                               {645, 361}, {693, 313}, {800, 315}, {800, GroundY}, {0, GroundY}});
    } else {
        p.drawPolygon(QPolygon{{0, 397}, {99, 324}, {165, 362}, {240, 310}, {287, 353},
                               {370, 288}, {465, 348}, {531, 333}, {633, 401}, {681, 350},
                               {800, 388}, {800, GroundY}, {0, GroundY}});
    }
    p.setBrush(palette.middleRidge);
    p.drawPolygon(QPolygon{{0, 412}, {104, 429}, {179, 383}, {268, 433}, {344, 420},
                           {441, 370}, {565, 440}, {661, 408}, {718, 437}, {800, 417},
                           {800, GroundY}, {0, GroundY}});

    p.setBrush(palette.foreground);
    p.drawPolygon(QPolygon{{0, 468}, {101, 445}, {188, 469}, {292, 456}, {408, 480},
                           {523, 466}, {623, 442}, {721, 464}, {800, 454},
                           {800, GroundY}, {0, GroundY}});
    if (stage.id == StageId::Bamboo) {
        for (int x : {29, 55, 79, 715, 746, 777}) {
            const int top = 210 + (x * 3) % 98;
            const int lean = x < 400 ? -14 : 14;
            p.setPen(QPen(palette.foreground.darker(115), 5));
            p.drawLine(x, GroundY, x + lean, top);
            p.setPen(QPen(palette.foreground.lighter(120), 2));
            for (int y = top + 34; y < GroundY - 28; y += 36)
                p.drawLine(x + lean / 2, y, x + 19 * (lean > 0 ? 1 : -1), y - 13);
        }
    }

    for (const QRect &platform : stage.platforms) {
        p.fillRect(platform, palette.floor.darker(118));
        p.fillRect(platform.x(), platform.y(), platform.width(), 2, palette.edge);
        p.fillRect(platform.x() + 5, platform.y() + 4, platform.width() - 10, 3, palette.floor.lighter(146));
        p.setPen(QPen(palette.edge.darker(150), 1));
        for (int x = platform.left() + 25; x < platform.right() - 8; x += 30)
            p.drawLine(x, platform.top() + 3, x - 3, platform.bottom() - 2);
    }

    p.fillRect(0, GroundY, Width, Height - GroundY, palette.foreground.darker(118));
    p.fillRect(0, GroundY, Width, 2, palette.edge);
    p.fillRect(0, GroundY + 2, Width, 23, palette.floor);
    p.setPen(palette.edge.darker(180));
    p.drawLine(0, GroundY + 24, Width, GroundY + 24);
    for (int x = 47; x < Width; x += 88)
        p.drawLine(x, GroundY + 3, x - 11, GroundY + 23);

    p.fillRect(0, 536, Width, 64, QColor("#191722"));
    p.setPen(QColor("#553e48"));
    p.drawLine(24, 537, Width - 24, 537);
}

void drawMenu(QPainter &p, const Fighter &playerOne, const Fighter &playerTwo, const Stage &stage, bool adjustingVolume)
{
    QLinearGradient background(0, 0, Width, Height);
    background.setColorAt(0, QColor("#171522"));
    background.setColorAt(1, QColor("#38252f"));
    p.fillRect(0, 0, Width, Height, background);

    text(p, QRect(52, 27, 45, 32), QStringLiteral("地图"), 15, Muted, true, Left);
    text(p, QRect(52, 95, 330, 78), QStringLiteral("格斗竞技场"), 54, Ink, true, Left);
    p.fillRect(QRect(53, 188, 39, 3), Gold);

    const ArenaPalette palette = arenaPalette(stage.id);
    const QRect poster(382, 81, 366, 351);
    p.save();
    QPainterPath posterClip;
    posterClip.addRoundedRect(poster, 12, 12);
    p.setClipPath(posterClip);
    QLinearGradient backdrop(poster.topLeft(), poster.bottomRight());
    backdrop.setColorAt(0, palette.dusk);
    backdrop.setColorAt(1, palette.horizon.darker(126));
    p.fillRect(poster, backdrop);
    p.setPen(Qt::NoPen);
    p.setBrush(palette.light);
    p.drawEllipse(QPointF(570, 202), 79, 79);

    p.setBrush(palette.farRidge);
    p.drawPolygon(QPolygon{{382, 304}, {453, 250}, {512, 288}, {562, 258},
                           {614, 285}, {677, 238}, {748, 294}, {748, 362}, {382, 362}});
    p.setBrush(palette.middleRidge);
    p.drawPolygon(QPolygon{{382, 329}, {477, 313}, {546, 338}, {662, 305},
                           {748, 320}, {748, 367}, {382, 367}});
    p.setBrush(QColor(21, 17, 28, 125));
    p.drawEllipse(QPointF(480, 357), 54, 9);
    p.drawEllipse(QPointF(652, 357), 56, 9);
    playerOne.drawIdlePortrait(p, QRect(402, 184, 154, 173), true);
    playerTwo.drawIdlePortrait(p, QRect(575, 180, 152, 177), false);

    p.fillRect(QRect(382, 378, 366, 54), QColor("#231f2c"));
    p.setPen(QColor("#80616a"));
    p.drawLine(382, 378, 748, 378);
    p.drawLine(565, 392, 565, 418);
    text(p, QRect(393, 383, 168, 44), QStringLiteral("Martial Hero"), 18, Red, true);
    text(p, QRect(570, 383, 168, 44), QStringLiteral("Hero Knight"), 18, Blue, true);
    p.restore();
    p.setPen(QColor("#907078"));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(poster, 12, 12);

    const QString navigationHint = adjustingVolume
        ? QStringLiteral("方向键调节音量   ·   空格确认")
        : QStringLiteral("方向键选择   ·   空格确认");
    text(p, QRect(52, 502, 560, 34), navigationHint, 16, Muted, false, Left);
}

void drawHud(QPainter &p, const HealthBar &p1, const HealthBar &p2, int remainingMs, bool showMenuHint,
             const QString &opponentName)
{
    drawHealthCards(p, p1, p2, opponentName, Blue);
    const bool timeLow = remainingMs <= 10000;
    const QColor timerColor = timeLow ? Red : Gold;
    QColor timerBorder = Border;
    if (timeLow) {
        const qreal pulse = qAbs(remainingMs % 1000 - 500) / 500.0;
        timerBorder = Red;
        timerBorder.setAlpha(qRound(115 + 120 * pulse));
    }

    panel(p, QRect(350, 22, 100, 78), timerBorder);
    const int timeLeft = (remainingMs + 999) / 1000;
    const QString time = QStringLiteral("%1").arg(timeLeft, 2, 10, QLatin1Char('0'));
    text(p, QRect(350, 25, 100, 21), QStringLiteral("剩余时间"), 12, Muted);
    text(p, QRect(350, 43, 100, 48), time, 37, timerColor, true);
    if (showMenuHint) drawFooter(p, false);
}

void drawTrainingHud(QPainter &p, const HealthBar &p1, const HealthBar &dummy, int lastDamage, int totalDamage, const QString &dummyName)
{
    drawHealthCards(p, p1, dummy, dummyName, Wood);
    panel(p, QRect(346, 22, 108, 78));
    p.setPen(Border);
    p.drawLine(354, 60, 446, 60);
    text(p, QRect(354, 31, 49, 23), QStringLiteral("上次伤害"), 11, Muted, false, Left);
    text(p, QRect(404, 29, 41, 27), QString::number(lastDamage), 19, Gold, true, Right);
    text(p, QRect(354, 67, 49, 23), QStringLiteral("累计伤害"), 11, Muted, false, Left);
    text(p, QRect(403, 65, 42, 27), QString::number(totalDamage), totalDamage > 9999 ? 12 : 17,
         Ink, true, Right);
    keyHint(p, 299, 550, 24, QStringLiteral("T"), QStringLiteral("木桩模式"));
    drawFooter(p, true);
}

void drawTrainingDummy(QPainter &p, const QRect &body, int health, qreal flash,
                       bool facingRight, bool winding, const QRect &attackBox)
{
    p.save();
    p.translate(body.topLeft());
    if (health <= 0) p.setOpacity(0.45);
    const int w = body.width();
    const int h = body.height();
    p.fillRect(QRect(w / 2 - 4, 28, 8, h - 28), QColor("#89583f"));
    p.fillRect(QRect(6, h - 5, w - 12, 5), QColor("#aa7050"));

    const QRectF board(1, 1, w - 2, h - 13);
    p.setPen(QPen(QColor("#784a37"), 2));
    p.setBrush(QColor("#d39d63"));
    p.drawRoundedRect(board, 6, 6);
    p.setPen(QPen(QColor("#a45845"), 2));
    p.setBrush(QColor("#eab77a"));
    const QPointF center = board.center();
    p.drawEllipse(center, 13, 13);
    p.drawEllipse(center, 5, 5);

    if (flash > 0) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 244, 215, qRound(180 * flash)));
        p.drawRoundedRect(board, 6, 6);
    }
    p.restore();

    if (!winding || health <= 0) return;
    p.save();
    p.translate(body.center());
    p.scale(facingRight ? 1 : -1, 1);
    if (attackBox.isEmpty()) {
        p.setPen(QPen(Gold, 2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(27, -12), QPointF(27, -3));
        p.drawPoint(QPointF(27, 2));
        p.drawLine(QPointF(18, 5), QPointF(38, -10));
    } else {
        QPainterPath slash;
        slash.moveTo(19, 19);
        slash.quadTo(50, 17, 66, -17);
        p.setPen(QPen(QColor("#d47551"), 5, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(slash);
        p.setPen(QPen(Gold, 2));
        p.drawPath(slash);
    }
    p.restore();
}

void drawHitboxes(QPainter &p, const QRect &p1Body, const QRect &p2Body,
                  const QRect &p1Attack, const QRect &p2Attack, const QRect &lastHit)
{
    p.save();
    debugBox(p, p1Body, HurtboxColor);
    debugBox(p, p2Body, HurtboxColor);
    debugBox(p, p1Attack, HitboxColor);
    debugBox(p, p2Attack, HitboxColor);
    debugBox(p, lastHit, Gold, Qt::DashLine);
    p.fillRect(QRect(25, 557, 8, 8), HurtboxColor);
    p.fillRect(QRect(104, 557, 8, 8), HitboxColor);
    p.setPen(QPen(Gold, 1, Qt::DashLine));
    p.drawLine(185, 561, 199, 561);
    text(p, QRect(40, 550, 62, 24), QStringLiteral("受击框"), 13, Ink, false, Left);
    text(p, QRect(119, 550, 66, 24), QStringLiteral("攻击框"), 13, Ink, false, Left);
    text(p, QRect(205, 550, 63, 24), QStringLiteral("命中瞬间"), 13, Ink, false, Left);
    p.restore();
}

void drawBlinkTrail(QPainter &p, const Blink &blink, bool firstPlayer)
{
    if (blink.trailTicks() <= 0) return;

    const QPointF start = blink.trailStart();
    const QPointF movement = QPointF(blink.trailEnd()) - start;
    const qreal opacity = qreal(blink.trailTicks()) / Blink::TrailTicks;
    const QColor accent = firstPlayer ? Red : Blue;

    p.save();
    p.setOpacity(opacity * 0.75);
    p.setPen(QPen(accent, 2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(start + QPointF(0, -8), start + movement * 0.86 + QPointF(0, -8));
    p.drawLine(start + movement * 0.16, start + movement);
    p.drawLine(start + movement * 0.08 + QPointF(0, 8), start + movement * 0.76 + QPointF(0, 8));
    p.setPen(Qt::NoPen);
    for (int step = 1; step <= 3; ++step) {
        QColor trace = accent;
        trace.setAlpha(26 + step * 22);
        p.setBrush(trace);
        p.drawEllipse(start + movement * (step * 0.23), 9 + step * 2, 15 + step);
    }
    p.restore();
}

void drawBlinkStatus(QPainter &p, const Blink &p1, const Blink &p2, bool showP2)
{
    const auto draw = [&](const QRect &area, const Blink &blink, bool first) {
        const int cooldown = blink.cooldown();
        const bool followUp = blink.followUpTicks() > 0;
        const QString label = followUp ? (first ? QStringLiteral("影切") : QStringLiteral("破阵"))
                                       : QStringLiteral("瞬身");
        const QColor accent = followUp ? (first ? SwordColor : Gold) : Blue;
        const QString status = followUp ? QStringLiteral("按普攻") : cooldown > 0
            ? QStringLiteral("%1 s").arg(cooldown * CombatTickMs / 1000.0, 0, 'f', 1)
            : QStringLiteral("就绪");
        panel(p, area, followUp ? accent : Border);
        text(p, area.adjusted(9, 0, -52, 0), label, 12, accent, true, Left);
        text(p, area.adjusted(43, 0, -9, 0), status, 12, followUp || cooldown == 0 ? Ink : Muted, true, Right);
        const qreal remaining = followUp ? qreal(blink.followUpTicks()) / Blink::FollowUpTicks
                                       : 1.0 - qreal(cooldown) / Blink::CooldownTicks;
        const int fill = qRound((area.width() - 14) * remaining);
        p.fillRect(QRect(area.left() + 7, area.bottom() - 3, fill, 2), accent);
    };

    draw(QRect(181, 108, 100, 26), p1, true);
    if (showP2) draw(QRect(519, 108, 100, 26), p2, false);
}

void drawLungeEffect(QPainter &p, const Fighter &fighter, SkillKind kind, int elapsed)
{
    const auto spec = lungeSpec(kind);
    const bool knight = kind == SkillKind::Lightning;
    const int travelEnd = spec.windup + spec.travel;
    const QRect body = fighter.getBodyBox();
    p.save();
    p.translate(body.center());
    p.scale(fighter.facingRight ? 1 : -1, 1);

    if (elapsed <= spec.windup) {
        p.setPen(QPen(knight ? Gold : SwordColor, 2, Qt::SolidLine, Qt::RoundCap));
        p.setOpacity(0.45 + 0.5 * elapsed / spec.windup);
        p.drawLine(QPointF(9, -2), QPointF(knight ? 35 : 44, knight ? -14 : -2));
    } else {
        const qreal recover = qBound(0.0, qreal(elapsed - travelEnd) / (spec.duration - travelEnd), 1.0);
        p.setOpacity(1.0 - recover);
        if (knight) {
            QPainterPath arc;
            arc.moveTo(13, 20);
            arc.quadTo(57, 16, 52, -19);
            arc.quadTo(48, -31, 30, -34);
            p.setPen(QPen(QColor("#56949d"), 7, Qt::SolidLine, Qt::RoundCap));
            p.drawPath(arc);
            p.setPen(QPen(QColor("#e8f5ea"), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawPath(arc);
            p.setPen(QPen(Gold, 2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(40, 16), QPointF(53, 23));
            p.drawLine(QPointF(48, 10), QPointF(61, 10));
            p.drawLine(QPointF(49, 3), QPointF(60, -4));
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#9060d8"));
            p.drawPolygon(QPolygon{{-50, 9}, {16, 0}, {66, -9}, {61, -3}, {25, 5}});
            p.setPen(QPen(QColor("#ffd3ff"), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(-24, 6), QPointF(59, -6));
            p.setPen(QPen(QColor("#b486ff"), 1, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(-51, 15), QPointF(42, 4));
            p.drawLine(QPointF(-37, -9), QPointF(8, -13));
        }
    }
    p.restore();
}

void drawGuardEffect(QPainter &p, const Fighter &fighter, bool guarding)
{
    if (!guarding && fighter.blockFlashTicks <= 0) return;

    const QRect body = fighter.getBodyBox();
    const int direction = fighter.facingRight ? 1 : -1;
    const int x = fighter.facingRight ? body.right() + 4 : body.left() - 4;
    const int y = body.center().y();
    p.save();
    p.translate(x, y);
    p.scale(direction, 1);
    if (guarding) {
        p.setPen(QPen(QColor(169, 224, 223, 140), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(-21, -24, 32, 48), -64 * 16, 128 * 16);
    }
    if (fighter.blockFlashTicks > 0) {
        QColor spark = LightningColor;
        spark.setAlpha(qRound(255.0 * fighter.blockFlashTicks / Fighter::HitFlashTicks));
        p.setPen(QPen(spark, 2));
        p.drawLines(QVector<QLineF>{{7, -5, 20, -15}, {12, 0, 27, 0},
                                    {7, 5, 20, 15}, {6, -9, 9, -18}, {6, 9, 9, 18}});
    }
    p.restore();
}

void drawCounterEffects(QPainter &p, const Fighter &fighter, const PlayerLogic &logic)
{
    const int flash = logic.perfectFlashTicks();
    if (!logic.isCountering() && flash == 0) return;

    const bool knight = logic.skill() == SkillKind::Lightning;
    const QColor accent = knight ? Gold : QColor("#9cefff");
    const QRect body = fighter.getBodyBox();
    p.save();
    p.translate(body.center());
    p.scale(fighter.facingRight ? 1 : -1, 1);

    if (flash > 0) {
        p.save();
        const qreal progress = 1.0 - qreal(flash) / PlayerLogic::PerfectFlashTicks;
        p.setOpacity(1.0 - progress);
        p.setPen(QPen(accent, 2));
        p.setBrush(Qt::NoBrush);
        const qreal radius = 20 + 22 * progress;
        p.drawEllipse(QPointF(14, 0), radius, radius);
        p.setPen(QPen(QColor("#fff8da"), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(23, -12), QPointF(36, -22));
        p.drawLine(QPointF(25, 3), QPointF(44, 1));
        p.drawLine(QPointF(19, 12), QPointF(32, 21));
        p.restore();
    }

    if (logic.isCountering()) {
        const auto spec = counterSpec(logic.skill());
        const int tick = logic.counterTick();
        if (logic.hasCounterArmor()) {
            QColor shell = accent;
            shell.setAlpha(74);
            p.setPen(QPen(accent, 1.5));
            p.setBrush(shell);
            p.drawRoundedRect(QRectF(-body.width() / 2.0 - 5, -body.height() / 2.0 - 3,
                                      body.width() + 10.0, body.height() + 6.0), 9, 9);
        }
        if (tick > spec.windup) {
            const qreal fade = qBound(0.0, (spec.duration - tick) / qreal(spec.duration - spec.windup), 1.0);
            p.setOpacity(fade);
            QPainterPath slash;
            if (knight) {
                slash.moveTo(11, 22);
                slash.cubicTo(40, 24, 61, -5, 61, -27);
                p.setPen(QPen(QColor("#ba713a"), 9, Qt::SolidLine, Qt::RoundCap));
                p.drawPath(slash);
            } else {
                slash.moveTo(4, 13);
                slash.quadTo(28, -17, 63, -9);
                p.setPen(QPen(QColor("#3985a8"), 6, Qt::SolidLine, Qt::RoundCap));
                p.drawPath(slash);
            }
            p.setPen(QPen(accent, knight ? 3 : 2, Qt::SolidLine, Qt::RoundCap));
            p.drawPath(slash);
        }
    }

    p.restore();
    if (flash > 0)
        text(p, QRect(body.center().x() - 60, body.top() - 43, 120, 24),
             QStringLiteral("精准格挡"), 14, accent, true);
}

void drawCounterStatus(QPainter &p, const PlayerLogic &p1, const PlayerLogic &p2, bool showP2)
{
    const auto draw = [&](const QRect &area, const PlayerLogic &logic) {
        const bool ready = logic.counterReadyTicks() > 0;
        const bool armored = logic.hasCounterArmor();
        if (!ready && !armored) return;
        const bool knight = logic.skill() == SkillKind::Lightning;
        const QColor accent = knight ? Gold : QColor("#9cefff");
        panel(p, area, accent);
        text(p, area.adjusted(9, 0, -59, 0), knight ? QStringLiteral("铁壁回斩") : QStringLiteral("截刃"),
             12, accent, true, Left);
        text(p, area.adjusted(66, 0, -9, 0), ready ? QStringLiteral("按普攻") : QStringLiteral("霸体"),
             12, Ink, true, Right);
        if (ready) {
            const int fill = (area.width() - 14) * logic.counterReadyTicks() / PlayerLogic::CounterReadyTicks;
            p.fillRect(QRect(area.x() + 7, area.bottom() - 3, fill, 2), accent);
        }
    };
    draw(QRect(25, 141, 149, 26), p1);
    if (showP2) draw(QRect(626, 141, 149, 26), p2);
}

void drawSkillEffect(QPainter &p, const SkillEffect &effect)
{
    if (effect.lifetime <= 0) return;
    p.save();

    if (effect.kind == SkillKind::SwordWave) {
        if (effect.hit) p.setOpacity(qreal(effect.lifetime) / SkillEffect::ImpactTicks);
        p.translate(effect.area.center());
        p.scale(effect.direction, 1);
        QPainterPath wave;
        wave.moveTo(-30, 9);
        wave.quadTo(-4, 2, 15, -12);
        wave.quadTo(29, -11, 30, -3);
        wave.quadTo(12, 12, -30, 9);
        p.setPen(QPen(QColor("#8c53d6"), 2));
        p.setBrush(QColor("#a268ec"));
        p.drawPath(wave);
        p.setPen(QPen(QColor("#ead6ff"), 2));
        p.drawLine(QPointF(-9, 5), QPointF(23, -7));
    } else if (effect.kind == SkillKind::Lightning) {
        const QRect box = effect.area;
        const int x = box.center().x();
        const int top = box.top();
        const int ground = box.bottom();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 214, 112, effect.lifetime * 8));
        p.drawEllipse(QRect(box.left(), ground - 12, box.width(), 24));

        const QPolygon bolt{{x + 17, top}, {x - 18, top + 43}, {x + 4, top + 43},
                             {x - 12, ground - 10}, {x + 28, top + 31}, {x + 10, top + 31},
                             {x + 35, top}};
        p.setPen(QPen(QColor("#e8aa52"), 1.5));
        p.setBrush(LightningColor);
        p.drawPolygon(bolt);

        p.setPen(QPen(LightningColor, 2));
        p.drawPolyline(QPolygon{{x - 9, ground - 8}, {x - 27, ground - 31},
                                  {x - 43, ground - 29}, {x - 57, ground - 44}});
        p.drawPolyline(QPolygon{{x + 8, ground - 4}, {x + 26, ground - 27},
                                  {x + 44, ground - 25}, {x + 61, ground - 41}});
    }
    p.restore();
}

void drawHitSpark(QPainter &p, const QPoint &contact, qreal age)
{
    p.save();
    p.translate(contact);
    p.setOpacity(1.0 - age);
    p.scale(0.8 + age * 1.2, 0.8 + age * 1.2);
    p.setPen(QPen(QColor("#ff9a54"), 2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPoint(-9, -4), QPoint(-19, -10));
    p.drawLine(QPoint(8, -6), QPoint(17, -15));
    p.drawLine(QPoint(9, 4), QPoint(21, 7));
    p.drawLine(QPoint(-5, 7), QPoint(-12, 19));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#fff2bc"));
    p.drawPolygon(QPolygon{{0, -11}, {4, -4}, {12, 0}, {5, 3},
                           {0, 10}, {-4, 4}, {-11, 0}, {-4, -4}});
    p.restore();
}

void drawSkillHitboxes(QPainter &p, const QRect &p1Skill, const QRect &p2Skill)
{
    p.save();
    debugBox(p, p1Skill, SwordColor);
    debugBox(p, p2Skill, LightningColor);
    p.restore();
}

void drawSkillStatus(QPainter &p, SkillKind p1, int p1Cooldown, SkillKind p2, int p2Cooldown)
{
    const auto draw = [&](const QRect &area, SkillKind kind, int cooldown) {
        if (kind == SkillKind::None) return;
        const bool sword = kind == SkillKind::SwordWave;
        const QColor color = sword ? SwordColor : LightningColor;
        const QString name = sword ? QStringLiteral("紫色剑气") : QStringLiteral("近身落雷");
        const QString status = cooldown > 0
            ? QStringLiteral("%1 s").arg(cooldown * CombatTickMs / 1000.0, 0, 'f', 1)
            : QStringLiteral("就绪");

        panel(p, area);
        text(p, area.adjusted(9, 0, -48, 0), name, 12, color, true, Left);
        text(p, area.adjusted(86, 0, -9, 0), status, 12, cooldown > 0 ? Muted : Ink, true, Right);

        const int available = skillSpec(kind).cooldown - cooldown;
        const int width = (area.width() - 14) * available / skillSpec(kind).cooldown;
        p.fillRect(QRect(area.left() + 7, area.bottom() - 3, width, 2), color);
    };

    draw(QRect(25, 108, 149, 26), p1, p1Cooldown);
    draw(QRect(626, 108, 149, 26), p2, p2Cooldown);
}

void drawRoundStart(QPainter &p, int countdown)
{
    if (countdown > 0) {
        panel(p, QRect(333, 193, 134, 147));
        p.fillRect(QRect(381, 193, 38, 2), Gold);
        text(p, QRect(333, 206, 134, 28), QStringLiteral("准备"), 16, Muted, true);
        text(p, QRect(333, 236, 134, 88), QString::number(countdown), 76, Gold, true);
    } else {
        panel(p, QRect(301, 228, 198, 76), Gold);
        text(p, QRect(301, 230, 198, 72), QStringLiteral("开始！"), 38, Gold, true);
    }
}

void drawResult(QPainter &p, Winner winner, const QString &detail, const QString &opponentName)
{
    QString title = QStringLiteral("平局");
    QColor accent = Gold;
    if (winner == Winner::PlayerOne) {
        title = QStringLiteral("玩家 1 获胜");
        accent = Red;
    } else if (winner == Winner::PlayerTwo) {
        title = QStringLiteral("%1 获胜").arg(opponentName);
        accent = Blue;
    }

    p.fillRect(0, 0, Width, Height, QColor(13, 10, 20, 177));
    panel(p, QRect(143, 153, 514, 296));
    p.fillRect(QRect(371, 154, 58, 3), accent);
    text(p, QRect(163, 185, 474, 77), title, 48, accent, true);
    text(p, QRect(163, 267, 474, 30), detail, 18, Ink);
    p.setPen(Border);
    p.drawLine(174, 316, 626, 316);
    text(p, QRect(169, 399, 462, 30), QStringLiteral("空格 再战   ·   Esc 返回主菜单"), 15, Muted);
}

} // namespace GameUI
