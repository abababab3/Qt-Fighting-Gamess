#include "hero.h"

#include <QPixmap>

#include "fighter.h"

const HeroTraits &heroTraits(HeroKind kind)
{
    // Martial Hero：中距离剑客，剑气远程消耗，帧图 200x200
    static const HeroTraits martial{SkillKind::SwordWave, 200, 0, 0.30, "p1", QColor("#e4564c")};
    // Hero Knight：拆盾特化，落雷高破盾，反击范围更大，帧图 180x180
    static const HeroTraits knight{SkillKind::Lightning, 180, 1, 0.35, "p2", QColor("#4a7fd6")};
    return kind == HeroKind::HeroKnight ? knight : martial;
}

QString heroName(HeroKind kind)
{
    return kind == HeroKind::HeroKnight ? QStringLiteral("Hero Knight")
                                        : QStringLiteral("Martial Hero");
}

QString heroSkillName(HeroKind kind)
{
    return kind == HeroKind::HeroKnight ? QStringLiteral("落雷") : QStringLiteral("剑气");
}

QString heroCounterName(HeroKind kind)
{
    return kind == HeroKind::HeroKnight ? QStringLiteral("铁壁回斩") : QStringLiteral("截刃");
}

QString heroBlurb(HeroKind kind)
{
    if (kind == HeroKind::HeroKnight)
        return QStringLiteral("拆盾特化 · 反击范围更大\n盾碎后一刀吃满伤害");
    return QStringLiteral("中距离剑客 · 剑气消耗\n起手更快，连段更顺");
}

void loadHeroAssets(Fighter &fighter, HeroKind kind)
{
    const HeroTraits &traits = heroTraits(kind);
    const QString base = QStringLiteral(":/res/") + QLatin1String(traits.prefix);
    const int s = traits.frameSize;

    // frameCount 只是兜底：loadFrames 内部按 图宽/单帧宽 重新推导真实帧数
    fighter.loadFrames(AnimState::Idle,    base + QStringLiteral("_ldle.png"),    8, s, s);
    fighter.loadFrames(AnimState::Walk,    base + QStringLiteral("_walk.png"),    8, s, s);
    fighter.loadFrames(AnimState::Jump,    base + QStringLiteral("_jump.png"),    3, s, s);
    fighter.loadFrames(AnimState::Attack,  base + QStringLiteral("_attack1.png"), 7, s, s);
    fighter.loadFrames(AnimState::Attack2, base + QStringLiteral("_attack2.png"), 7, s, s);
    fighter.loadFrames(AnimState::Hurt,    base + QStringLiteral("_hurt.png"),    4, s, s);
    fighter.loadFrames(AnimState::Guard,   base + QStringLiteral("_guard.png"),   1, s, s, 2);

    const QPixmap lungeAtlas(QStringLiteral(":/res/lunge_poses.png"));
    fighter.loadLungePose(lungeAtlas, traits.lungeColumn, traits.lungeAnchor);
}
