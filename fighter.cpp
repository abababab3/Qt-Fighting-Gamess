#include "fighter.h"
#include <QImage>
#include <algorithm>

namespace {

QRect opaqueBounds(const QImage &image, const QRect &region)
{
    int left = region.right() + 1;
    int top = region.bottom() + 1;
    int right = -1;
    int bottom = -1;
    for (int y = region.top(); y <= region.bottom(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = region.left(); x <= region.right(); ++x) {
            if (qAlpha(line[x]) <= 8) continue;
            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }
    if (right < 0) return {};
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

QPixmap atlasPose(const QPixmap &source, const QImage &image, const QRect &cell, int height, int xOffset = 0)
{
    constexpr int frame = 180;
    QPixmap pose(frame, frame);
    pose.fill(Qt::transparent);
    const QRect bounds = opaqueBounds(image, cell);
    if (bounds.isEmpty()) return pose;

    const auto sprite = source.copy(bounds).scaledToHeight(height, Qt::FastTransformation);
    QPainter painter(&pose);
    painter.drawPixmap(frame / 2 - sprite.width() / 2 + xOffset, 120 - height, sprite);
    return pose;
}

QPixmap poseStrip(const QPixmap &idle, const QPixmap &action)
{
    const int frame = idle.width();
    QPixmap strip(frame * 4, idle.height());
    strip.fill(Qt::transparent);
    QPainter painter(&strip);
    painter.drawPixmap(0, 0, idle);
    painter.drawPixmap(frame, 0, action);
    painter.drawPixmap(frame * 2, 0, action);
    painter.drawPixmap(frame * 3, 0, idle);
    return strip;
}

} // namespace

Fighter::Fighter(const QSize &bodySize)
{
    std::fill_n(m_frameW, ANIM_STATE_COUNT, bodySize.width());
    std::fill_n(m_frameH, ANIM_STATE_COUNT, bodySize.height());
}

bool Fighter::loadFrames(AnimState state, const QString &path,
                         int frameCount, int fw, int fh, int singleFrame) {
    return loadFrames(state, QPixmap(path), frameCount, fw, fh, singleFrame);
}

bool Fighter::loadFrames(AnimState state, QPixmap sheet, int frameCount, int fw, int fh, int singleFrame)
{
    if (sheet.isNull() || fw <= 0 || fh <= 0) return false;
    if (singleFrame >= 0) {
        const QRect frame(singleFrame * fw, 0, fw, fh);
        if (!sheet.rect().contains(frame)) return false;
        sheet = sheet.copy(frame);
    }
    int i = static_cast<int>(state);

    // 帧数由「图片宽度 / 单帧宽度」自动推导，调用方传的 frameCount 只做兜底。
    // 原来照 frameCount 算切帧宽度（width()/frameCount），一旦填错就会把
    // 相邻两帧一起切进 src —— 这就是“重影”的根因。
    int n = sheet.width() / fw;
    if (n <= 0) n = frameCount;
    if (n <= 0) return false;

    m_sheet[i]      = sheet;
    m_flashSheet[i] = sheet;
    QPainter flashPainter(&m_flashSheet[i]);
    flashPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    flashPainter.fillRect(sheet.rect(), Qt::white);
    flashPainter.end();
    m_frameCount[i] = n;
    m_frameW[i]     = fw;
    m_frameH[i]     = fh;

    // 扫描每帧 alpha，取所有帧可见像素框的并集（转成帧内相对坐标）作为身体框。
    // 这些图里 200x200 的帧实际只有约 40x53 是角色，底部还有 78px 透明留白，
    // 不处理的话身体框/攻击框/落地判定会全部错位。
    const QImage img = sheet.toImage().convertToFormat(QImage::Format_ARGB32);
    QRect uni;
    for (int k = 0; k < n; ++k) {
        int x0 = k * fw;
        const QRect frame = QRect(x0, 0, fw, fh).intersected(img.rect());
        const QRect bounds = opaqueBounds(img, frame).translated(-x0, 0);
        if (bounds.isValid()) uni = uni.united(bounds);
    }
    m_bodyRect[i] = uni.isNull() ? QRect(0, 0, fw, fh) : uni;
    return true;
}

void Fighter::loadNinjaPoses(const QPixmap &atlas)
{
    if (atlas.isNull()) return;
    m_fixedBody = true;
    constexpr int frame = 180;
    const QImage image = atlas.toImage().convertToFormat(QImage::Format_ARGB32);
    // The throwing hand reaches a little past the nominal third quarter.
    const int edges[]{0, atlas.width() * 23 / 100, atlas.width() * 49 / 100,
                      atlas.width() * 79 / 100, atlas.width()};
    QPixmap poses[4];
    for (int i = 0; i < 4; ++i) {
        const QRect cell(edges[i], 0, edges[i + 1] - edges[i], atlas.height());
        const int height = i == 1 ? 47 : i == 3 ? 44 : 53;
        poses[i] = atlasPose(atlas, image, cell, height, i == 2 ? 7 : 0);
    }
    const QPixmap toss = poseStrip(poses[0], poses[2]);
    loadFrames(AnimState::Idle, poses[0], 1, frame, frame, -1);
    loadFrames(AnimState::Walk, poses[1], 1, frame, frame, -1);
    loadFrames(AnimState::Jump, poses[1], 1, frame, frame, -1);
    loadFrames(AnimState::Attack, toss, 4, frame, frame, -1);
    loadFrames(AnimState::Attack2, toss, 4, frame, frame, -1);
    loadFrames(AnimState::Hurt, poses[3], 1, frame, frame, -1);
    loadFrames(AnimState::Guard, poses[3], 1, frame, frame, -1);
}

void Fighter::loadWindPoses(const QPixmap &atlas)
{
    if (atlas.isNull()) return;
    m_fixedBody = true;
    constexpr int frame = 180;
    const QImage image = atlas.toImage().convertToFormat(QImage::Format_ARGB32);
    const int edges[]{0, 16, 36, 61, 80, 100};
    const int heights[]{53, 48, 48, 71, 53};
    const int offsets[]{0, 0, 9, 1, 8};
    QPixmap poses[5];
    for (int i = 0; i < 5; ++i) {
        const int left = edges[i] * atlas.width() / 100;
        const int right = edges[i + 1] * atlas.width() / 100;
        poses[i] = atlasPose(atlas, image, QRect(left, 0, right - left, atlas.height()), heights[i], offsets[i]);
    }

    loadFrames(AnimState::Idle, poses[0], 1, frame, frame, -1);
    loadFrames(AnimState::Walk, poses[1], 1, frame, frame, -1);
    loadFrames(AnimState::Jump, poses[1], 1, frame, frame, -1);
    loadFrames(AnimState::Attack, poseStrip(poses[0], poses[2]), 4, frame, frame, -1);
    loadFrames(AnimState::Attack2, poseStrip(poses[0], poses[4]), 4, frame, frame, -1);
    loadFrames(AnimState::Hurt, poses[1], 1, frame, frame, -1);
    loadFrames(AnimState::Guard, poses[0], 1, frame, frame, -1);
    loadFrames(AnimState::GlidePose, poses[3], 1, frame, frame, -1);
    loadFrames(AnimState::SweepPose, poses[4], 1, frame, frame, -1);
}

void Fighter::loadLungePose(const QPixmap &atlas, int column, qreal anchor)
{
    if (atlas.isNull()) return;
    const int width = atlas.width() / 2;
    const QImage image = atlas.toImage().convertToFormat(QImage::Format_ARGB32);
    const QRect bounds = opaqueBounds(image, QRect(column * width, 0, width, atlas.height()));
    if (!bounds.isValid()) return;

    m_lungePose = atlas.copy(bounds);
    m_lungeAnchor = anchor;
}

void Fighter::setState(AnimState s) {
    if (m_state == s) return;
    m_state = s;
    m_curFrame = 0;
    m_tick = 0;
}

AnimState Fighter::visualState() const
{
    if (m_state == AnimState::Skill || m_state == AnimState::Lunge) return AnimState::Attack2;
    if (m_state == AnimState::Counter) return AnimState::Attack;
    if (m_state == AnimState::Guard && getFrameCount(AnimState::Guard) == 0) return AnimState::Idle;
    return m_state;
}

void Fighter::updateAnimation() {
    const int i = static_cast<int>(visualState());
    if (m_frameCount[i] <= 1) return;
    if (++m_tick >= FRAME_DELAY) {
        m_tick = 0;
        m_curFrame++;
        if (m_curFrame >= m_frameCount[i]) {
            // 两组攻击和 Hurt 播完停在末帧，其余循环
            if (m_state == AnimState::Attack || m_state == AnimState::Attack2
                || m_state == AnimState::Hurt || m_state == AnimState::Skill || m_state == AnimState::Lunge || m_state == AnimState::Counter)
                m_curFrame = m_frameCount[i] - 1;
            else
                m_curFrame = 0;
        }
    }
}

int Fighter::footGap(AnimState state) const {
    int i = static_cast<int>(state);
    if (!m_bodyRect[i].isValid()) return 0;
    int gap = m_frameH[i] - m_bodyRect[i].bottom() - 1;
    return gap > 0 ? gap : 0;
}

void Fighter::draw(QPainter &p, DrawPose pose) const
{
    if ((m_state == AnimState::Lunge || pose == DrawPose::Thrust) && !m_lungePose.isNull()) {
        const QRect body = getBodyBox();
        const int height = qRound(body.height() * 0.91);
        const int width = qRound(qreal(m_lungePose.width()) * height / m_lungePose.height());
        p.save();
        p.translate(body.center().x(), body.bottom() - height + 1);
        p.scale(facingRight ? 1 : -1, 1);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        p.drawPixmap(QRect(-qRound(width * m_lungeAnchor), 0, width, height), m_lungePose);
        p.restore();
        return;
    }

    AnimState visual = visualState();
    if (pose == DrawPose::Guard && getFrameCount(AnimState::Guard) > 0)
        visual = AnimState::Guard;
    else if (pose == DrawPose::Glide && getFrameCount(AnimState::GlidePose) > 0)
        visual = AnimState::GlidePose;
    else if (pose == DrawPose::WindSweep && getFrameCount(AnimState::SweepPose) > 0)
        visual = AnimState::SweepPose;
    else if (pose == DrawPose::Run && getFrameCount(AnimState::Walk) > 0)
        visual = AnimState::Walk;
    const int i = static_cast<int>(visual);
    if (visual == AnimState::Guard) {
        const QRect body = getBodyBox();
        const QRect source = m_bodyRect[i];
        const int width = qRound(qreal(source.width()) * body.height() / source.height());
        p.save();
        p.translate(body.center().x() - width / 2 + (facingRight ? 3 : -3), body.top());
        if (!facingRight) {
            p.translate(width, 0);
            p.scale(-1, 1);
        }
        p.drawPixmap(QRect(0, 0, width, body.height()), m_sheet[i], source);
        p.restore();
        return;
    }
    const int w = m_frameW[i];
    const int h = m_frameH[i];
    const QRect target(0, 0, w, h);
    const qreal flashOpacity = qreal(flashTicks) / HitFlashTicks;

    p.save();
    p.translate(static_cast<int>(x), static_cast<int>(y + footGap(visual)));
    if (!facingRight) {
        // 精灵图里的角色没有在帧内水平居中（p1 偏左、p2 偏右各约 6px）。
        // 若绕帧中心镜像，朝左时整张图会平移「两倍偏心量」（骑士约 11px），
        // 而 getBodyBox() 是按未镜像的帧内包围盒算的 —— 贴图会和判定/影子/头顶标签错位。
        // 改绕 Idle 帧包围盒中心镜像：两个朝向的贴图落在同一个身体位置上。
        const QRect idleBox = m_bodyRect[static_cast<int>(AnimState::Idle)];
        const int axis = idleBox.isValid() ? idleBox.center().x() : w / 2;
        p.translate(axis * 2, 0);
        p.scale(-1, 1);
    }

    if (!m_sheet[i].isNull() && m_frameCount[i] > 0) {
        // The umbrella strike strip stores the extended weapon pose in its second frame.
        const int frame = pose == DrawPose::WindStrike ? 1 : m_curFrame;
        QRect src(qMin(frame, m_frameCount[i] - 1) * w, 0, w, h);
        src.setRight(qMin(src.right(), m_sheet[i].width() - 1));
        p.drawPixmap(target, m_sheet[i], src);
        if (flashTicks > 0) {
            p.setOpacity(p.opacity() * flashOpacity);
            p.drawPixmap(target, m_flashSheet[i], src);
        }
    } else {
        p.fillRect(target, placeholderColor);
        if (flashTicks > 0) {
            p.setOpacity(p.opacity() * flashOpacity);
            p.fillRect(target, Qt::white);
        }
    }
    p.restore();
}

void Fighter::drawIdlePortrait(QPainter &p, const QRect &area, bool faceRight) const
{
    const int idle = static_cast<int>(AnimState::Idle);
    const QRect source = m_bodyRect[idle];
    if (m_sheet[idle].isNull() || !source.isValid()) return;

    QSize size = source.size();
    size.scale(area.size(), Qt::KeepAspectRatio);
    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.translate(area.center().x() - size.width() / 2, area.y() + area.height() - size.height());
    if (!faceRight) {
        p.translate(size.width(), 0);
        p.scale(-1, 1);
    }
    p.drawPixmap(QRect(QPoint(), size), m_sheet[idle], source);
    p.restore();
}

int Fighter::getFrameWidth() const {
    return m_frameW[static_cast<int>(visualState())];
}

int Fighter::getFrameCount(AnimState s) const {
    return m_frameCount[static_cast<int>(s)];
}

QRect Fighter::getBodyBox() const {
    if (m_fixedBody || m_state == AnimState::Guard || m_state == AnimState::Skill || m_state == AnimState::Lunge || m_state == AnimState::Counter)
        return bodyBox(AnimState::Idle);
    return bodyBox(m_state);
}

QRect Fighter::bodyBox(AnimState state) const {
    int i = static_cast<int>(state);
    if (!m_bodyRect[i].isValid())
        return QRect(static_cast<int>(x), static_cast<int>(y), m_frameW[i], m_frameH[i]);
    return QRect(static_cast<int>(x + m_bodyRect[i].left()),
                 static_cast<int>(y + footGap(state) + m_bodyRect[i].top()),
                 m_bodyRect[i].width(), m_bodyRect[i].height());
}

QRect Fighter::getHitBox() const {
    if (!isAttackActive()) return QRect();
    return attackBox(m_state);
}

QRect Fighter::attackBox(AnimState attackState) const {
    // 判定盒的"起点"必须锚在躯干上，不能用攻击帧的 alpha 包围盒。
    // 挥剑帧的 bbox 里包含伸出去的剑，right() 比待机帧远 76px（P1：113→189），
    // 起点跟着前移，判定框就跳过身前一大段，表现成"贴脸反而打不到"。
    const QRect torso = bodyBox(AnimState::Idle);
    const QRect b = torso.isValid() ? torso : bodyBox(attackState);
    const int top = b.top() + b.height() / 8;
    const int h = b.height() * 3 / 4;

    // 剑尖实际能到多远：用攻击帧包围盒的前缘量出来，再留一点视觉余量。
    // 判定范围 = [躯干前缘, max(基础距离, 剑尖)]，近身到剑尖之间没有空洞，
    // 远端也不会超出剑太多（超出多少 = 这里的余量）。
    int reach = 75;
    const QRect blade = bodyBox(attackState);
    if (blade.isValid()) {
        const int origin = facingRight ? b.right() - 5 : b.left() + 5;
        const int tip = facingRight ? blade.right() : blade.left();
        reach = qMax(reach, qAbs(tip - origin) + 10);
    }
    if (facingRight)
        return QRect(b.right() - 5, top, reach, h);
    return QRect(b.left() - reach + 5, top, reach, h);
}

bool Fighter::isAttackActive() const {
    if (m_state != AnimState::Attack && m_state != AnimState::Attack2) return false;
    int i = static_cast<int>(m_state);
    if (m_frameCount[i] <= 1) return true;
    // 真正的出拳/踢腿在动画后半段（p1 是帧 4-5，p2 是帧 3-6）。
    // 原来用 f/3~2f/3 且攻击只持续 18 tick（m_curFrame 最多到 2），
    // 判定帧根本不会出现 —— 这就是“p1 连打击都没有”。
    return m_curFrame >= m_frameCount[i] / 2;
}

void Fighter::reset(double startX, double startY) {
    x = startX; y = startY; vx = 0; vy = 0;
    hp = maxHp; onGround = false;
    standingPlatform = ignoredPlatform = -1;
    flashTicks = blockFlashTicks = 0;
    m_state = AnimState::Idle; m_curFrame = 0; m_tick = 0;
}

void Fighter::faceTo(const Fighter &other) {
    if (other.x == x) return;   // 重合时保持原朝向，避免两人同时朝右
    facingRight = other.x > x;
}
