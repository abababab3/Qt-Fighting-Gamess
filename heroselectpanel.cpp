#include "heroselectpanel.h"

#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>

#include "fighter.h"
#include "gameui.h"

namespace {

// 与 gameui.cpp 保持一致的配色
const QColor Ink("#f6ebda");
const QColor Muted("#9a8790");
const QColor Gold("#f0d7b4");
const QColor Accent("#edb578");
const QColor Border("#5c4c51");

constexpr int CanvasW = GameUI::Width;
constexpr int CanvasH = GameUI::Height;

constexpr qreal PanelX = 70;
constexpr qreal PanelY = 84;
constexpr qreal PanelW = 660;
constexpr qreal PanelH = 432;

constexpr qreal CardW = 290;
constexpr qreal CardH = 250;
constexpr qreal CardTop = PanelY + 96;

constexpr qreal ButtonW = 110;
constexpr qreal ButtonH = 36;

const QRectF kPanel(PanelX, PanelY, PanelW, PanelH);

void text(QPainter &p, const QRectF &area, const QString &value, int size,
          const QColor &color, bool bold = false, int alignment = Qt::AlignCenter)
{
    QFont font = p.font();
    font.setPixelSize(size);
    font.setBold(bold);
    p.setFont(font);
    p.setPen(color);
    p.drawText(area, alignment, value);
}

// 立绘来源：两个角色各缓存一份只读 Fighter，避免每次绘制都重新解包精灵图
const Fighter &portraitOf(HeroKind kind)
{
    static Fighter martial;
    static Fighter knight;
    static bool loaded = false;
    if (!loaded) {
        loadHeroAssets(martial, HeroKind::MartialHero);
        loadHeroAssets(knight, HeroKind::HeroKnight);
        loaded = true;
    }
    return kind == HeroKind::HeroKnight ? knight : martial;
}

void drawCard(QPainter &p, const HeroSelect::Panel &panel, int index)
{
    const HeroKind kind = HeroSelect::heroAt(index);
    const HeroTraits &traits = heroTraits(kind);
    const bool selected = panel.cursor == index;
    const bool hovered = panel.hover == index;
    const QRectF card = HeroSelect::cardRect(index);

    p.setBrush(selected ? QColor("#312a3d") : (hovered ? QColor("#251f30") : QColor("#1d1826")));
    p.setPen(QPen(selected ? Accent : (hovered ? QColor("#8d7a92") : QColor("#3b3345")),
                  selected ? 2.0 : 1.0));
    p.drawRoundedRect(card, 10, 10);

    // 顶部角色主色条
    p.setPen(Qt::NoPen);
    p.setBrush(traits.accent);
    p.drawRoundedRect(QRectF(card.left() + 14, card.top() + 9, card.width() - 28, 3), 1.5, 1.5);

    // 立绘：左卡片朝右、右卡片朝左，和菜单里的站位一致
    const QRectF art(card.left() + 16, card.top() + 22, card.width() - 32, CardH - 126);
    portraitOf(kind).drawIdlePortrait(p, art.toAlignedRect(), index == 0);

    text(p, QRectF(card.left(), card.top() + 152, card.width(), 26), heroName(kind), 19,
         selected ? QColor("#ffe0b8") : Ink, true);
    text(p, QRectF(card.left() + 6, card.top() + 180, card.width() - 12, 20),
         QStringLiteral("%1 · %2").arg(heroSkillName(kind), heroCounterName(kind)), 13, Muted);
    text(p, QRectF(card.left() + 14, card.top() + 204, card.width() - 28, 40), heroBlurb(kind), 12,
         QColor("#7f6e7b"), false, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap);

    // 第二步里给“已经选好的那个角色”打个标记
    if (panel.opponentStep() && kind == panel.p1) {
        const QString badge = panel.mode == HeroSelect::Mode::VsLocal
                                  ? QStringLiteral("1P 已选") : QStringLiteral("你的角色");
        QFont badgeFont = p.font();
        badgeFont.setPixelSize(11);
        badgeFont.setBold(true);
        const int badgeWidth = QFontMetrics(badgeFont).horizontalAdvance(badge) + 16;
        const QRectF chip(card.right() - 12 - badgeWidth, card.top() + 13, badgeWidth, 20);
        QColor chipColor = traits.accent;
        chipColor.setAlpha(215);
        p.setPen(Qt::NoPen);
        p.setBrush(chipColor);
        p.drawRoundedRect(chip, 10, 10);
        text(p, chip, badge, 11, QColor("#14101a"), true);
    }
}

void drawButton(QPainter &p, const QRectF &rect, const QString &label, bool primary, bool hovered)
{
    QColor fill = primary ? Accent : QColor("#29242f");
    if (hovered) fill = primary ? QColor("#ffd2a0") : QColor("#3a2f39");
    p.setBrush(fill);
    p.setPen(QPen(primary ? Accent.lighter(108) : Border, 1.0));
    p.drawRoundedRect(rect, 8, 8);
    text(p, rect, label, 14, primary ? QColor("#251915") : Ink, true);
}

} // namespace

namespace HeroSelect {

HeroKind heroAt(int index)
{
    return index == 1 ? HeroKind::HeroKnight : HeroKind::MartialHero;
}

int indexOfHero(HeroKind kind)
{
    return kind == HeroKind::HeroKnight ? 1 : 0;
}

HeroKind Panel::cursorHero() const
{
    return heroAt(cursor);
}

void Panel::setCursorHero(HeroKind kind)
{
    cursor = indexOfHero(kind);
}

QRectF panelRect()
{
    return kPanel;
}

QRectF cardRect(int index)
{
    const qreal x = (index == 0) ? PanelX + 30 : PanelX + 340;
    return QRectF(x, CardTop, CardW, CardH);
}

QRectF cancelRect()
{
    return QRectF(PanelX + 398, PanelY + 360, ButtonW, ButtonH);
}

QRectF confirmRect()
{
    return QRectF(PanelX + 520, PanelY + 360, ButtonW, ButtonH);
}

int hitTest(const QPointF &pos)
{
    for (int i = 0; i < 2; ++i) {
        if (cardRect(i).contains(pos)) return i;
    }
    if (cancelRect().contains(pos)) return 2;
    if (confirmRect().contains(pos)) return 3;
    return -1;
}

void draw(QPainter &p, const Panel &panel)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    // 压暗整个战场，让面板是浮在游戏画面上的
    p.fillRect(QRectF(0, 0, CanvasW, CanvasH), QColor(8, 6, 12, 188));

    // 面板本体：上浅下深的竖向渐变 + 投影
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawRoundedRect(kPanel.translated(0, 7), 16, 16);

    QLinearGradient background(kPanel.topLeft(), kPanel.bottomLeft());
    background.setColorAt(0.0, QColor("#241d30"));
    background.setColorAt(1.0, QColor("#15111d"));
    p.setBrush(background);
    p.setPen(QPen(Border, 1.2));
    p.drawRoundedRect(kPanel, 16, 16);

    // 顶部高光条
    QLinearGradient topLine(kPanel.left() + 40, 0, kPanel.right() - 40, 0);
    topLine.setColorAt(0.0, QColor(237, 181, 120, 0));
    topLine.setColorAt(0.5, QColor(237, 181, 120, 190));
    topLine.setColorAt(1.0, QColor(237, 181, 120, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(topLine);
    p.drawRoundedRect(QRectF(kPanel.left() + 40, kPanel.top() + 1.5, kPanel.width() - 80, 2), 1, 1);

    const bool local = panel.mode == Mode::VsLocal;
    const bool single = panel.singleStep();
    QString title;
    QString subtitle;
    if (single) {
        title = QStringLiteral("选择英雄");
        subtitle = QStringLiteral("选择你要使用的角色");
    } else if (panel.opponentStep()) {
        title = local ? QStringLiteral("选择2P英雄") : QStringLiteral("选择对手");
        subtitle = local
            ? QStringLiteral("2P（右侧）选择你的角色   ·   1P 已选：%1").arg(heroName(panel.p1))
            : QStringLiteral("你的角色：%1   ·   选择电脑操控的角色").arg(heroName(panel.p1));
    } else {
        title = local ? QStringLiteral("选择1P英雄") : QStringLiteral("选择英雄");
        subtitle = local ? QStringLiteral("1P（左侧）选择你要操控的角色")
                         : QStringLiteral("选择你要操控的角色");
    }
    text(p, QRectF(kPanel.left() + 30, kPanel.top() + 20, 420, 34), title, 24, Gold, true,
         Qt::AlignLeft | Qt::AlignVCenter);
    // 单步流程没有「第几步」，不显示进度
    if (!single) {
        text(p, QRectF(kPanel.right() - 130, kPanel.top() + 24, 100, 26),
             QStringLiteral("%1 / 2").arg(panel.step + 1), 14, QColor("#6f5f6b"), true,
             Qt::AlignRight | Qt::AlignVCenter);
    }
    text(p, QRectF(kPanel.left() + 30, kPanel.top() + 58, 560, 20), subtitle, 13,
         QColor("#8b7883"), false, Qt::AlignLeft | Qt::AlignVCenter);

    drawCard(p, panel, 0);
    drawCard(p, panel, 1);

    text(p, QRectF(kPanel.left() + 30, cancelRect().top(), 340, ButtonH),
         panel.opponentStep() ? QStringLiteral("← → 切换   ·   空格 确认   ·   Esc 上一步")
                              : QStringLiteral("← → 切换   ·   空格 确认   ·   Esc 返回"),
         12, QColor("#77656f"), false, Qt::AlignLeft | Qt::AlignVCenter);

    drawButton(p, cancelRect(), panel.opponentStep() ? QStringLiteral("上一步") : QStringLiteral("返回"),
               false, panel.hover == 2);
    drawButton(p, confirmRect(),
               single ? QStringLiteral("开始练习")
                      : (panel.opponentStep() ? QStringLiteral("开始对战") : QStringLiteral("下一步")),
               true, panel.hover == 3);

    p.restore();
}

} // namespace HeroSelect
