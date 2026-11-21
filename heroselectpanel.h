#ifndef HEROSELECTPANEL_H
#define HEROSELECTPANEL_H

#include <QPointF>
#include <QRectF>

#include "hero.h"

class QPainter;

// 游戏内覆盖式选择面板（不是独立窗口）。
// 全部坐标都是画布坐标（800x600），由 MainWindow 负责把鼠标位置换算进来。
// 两步流程：step 0 先选一个角色，step 1 再选另一个（电脑或 2P 用）。
namespace HeroSelect {

// 面板用途：打电脑（选择英雄 / 选择对手）、本地双人（选择 1P / 2P 英雄）、
// 木桩练习（只有一步：选择英雄）
enum class Mode { VsComputer, VsLocal, Training };

struct Panel {
    Mode mode = Mode::VsComputer;
    int step = 0;                                        // 0 = 先选的角色，1 = 后选的角色
    int cursor = 0;                                      // 0 = 左卡片，1 = 右卡片
    int hover = -1;                                      // -1 无，0/1 卡片，2 取消，3 确认
    HeroKind p1 = HeroKind::MartialHero;                 // 已确定的玩家角色
    HeroKind p2 = HeroKind::HeroKnight;                  // 已确定的对手角色

    // 木桩练习没有对手可选，面板只有一步
    bool singleStep() const { return mode == Mode::Training; }
    bool opponentStep() const { return !singleStep() && step == 1; }
    HeroKind cursorHero() const;
    void setCursorHero(HeroKind kind);
};

// 卡片下标与英雄的固定对应关系（左 MartialHero，右 HeroKnight）
HeroKind heroAt(int index);
int indexOfHero(HeroKind kind);

QRectF panelRect();
QRectF cardRect(int index);
QRectF cancelRect();
QRectF confirmRect();

// 命中测试：返回 0/1 卡片、2 取消、3 确认、-1 无
int hitTest(const QPointF &pos);

// 在画布坐标下绘制遮罩 + 面板（调用方已经 translate/scale 到画布原点）
void draw(QPainter &p, const Panel &panel);

} // namespace HeroSelect

#endif // HEROSELECTPANEL_H
