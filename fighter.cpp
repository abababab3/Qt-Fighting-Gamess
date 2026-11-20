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

} // namespace

Fighter::Fighter(const QSize &bodySize)
{
    std::fill_n(m_frameW, ANIM_STATE_COUNT, bodySize.width());
    std::fill_n(m_frameH, ANIM_STATE_COUNT, bodySize.height());
}

bool Fighter::loadFrames(AnimState state, const QString &path,
                         int frameCount, int fw, int fh, int singleFrame) {
    QPixmap sheet(path);
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

void Fighter::draw(QPainter &p) const
{
    if (m_state == AnimState::Lunge && !m_lungePose.isNull()) {
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

    const AnimState visual = visualState();
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
        p.translate(w, 0);
        p.scale(-1, 1);
    }

    if (!m_sheet[i].isNull() && m_frameCount[i] > 0) {
        QRect src(m_curFrame * w, 0, w, h);
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
    if (m_state == AnimState::Guard || m_state == AnimState::Skill || m_state == AnimState::Lunge || m_state == AnimState::Counter)
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
    const QRect b = bodyBox(attackState);
    // 挥刀/挥剑的横向幅度大，判定框加宽加高一点，命中更明显
    int reach = 75;
    int top = b.top() + b.height() / 8;
    int h = b.height() * 3 / 4;
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
