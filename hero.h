#ifndef HERO_H
#define HERO_H

#include <QColor>
#include <QString>

#include "combatskill.h"

class Fighter;

// 可选角色。角色的全部差异最终收敛到三件事：
//   1. 精灵图资源（帧尺寸不同，身体框由 alpha 自动推出来）
//   2. 技能系（决定技能/突进/反击的数值表）
//   3. 突进图集的列号与锚点
// 所以「换角色」本质是把这三样重新灌进一个 Fighter + PlayerLogic。
enum class HeroKind { MartialHero, HeroKnight };

struct HeroTraits {
    SkillKind skill;
    int frameSize;      // 精灵单帧边长（p1=200，p2=180）
    int lungeColumn;    // lunge_poses 图集里的列
    qreal lungeAnchor;  // 突进姿势的贴图锚点
    const char *prefix; // 资源名前缀 "p1" / "p2"
    QColor accent;      // HUD / 卡片主色
};

const HeroTraits &heroTraits(HeroKind kind);
QString heroName(HeroKind kind);
QString heroSkillName(HeroKind kind);   // 技能名（落雷 / 剑气）
QString heroCounterName(HeroKind kind); // 完美格挡后的反击技名
QString heroBlurb(HeroKind kind);       // 一句话定位，用于选择窗口

// 把某个角色的全部动画帧灌进 fighter（会覆盖原有帧数据）
void loadHeroAssets(Fighter &fighter, HeroKind kind);

#endif // HERO_H
