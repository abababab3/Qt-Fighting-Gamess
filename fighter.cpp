#include "fighter.h"
#include <QImage>

Fighter::Fighter() {}

bool Fighter::loadFrames(AnimState state, const QString &path,
                         int frameCount, int fw, int fh) {
    QPixmap sheet(path);
    if (sheet.isNull() || fw <= 0 || fh <= 0) return false;
    int i = static_cast<int>(state);

    // 帧数由「图片宽度 / 单帧宽度」自动推导，调用方传的 frameCount 只做兜底。
    // 原来照 frameCount 算切帧宽度（width()/frameCount），一旦填错就会把
    // 相邻两帧一起切进 src —— 这就是“重影”的根因。
    int n = sheet.width() / fw;
    if (n <= 0) n = frameCount;
    if (n <= 0) return false;

    m_sheet[i]      = sheet;
    m_frameCount[i] = n;
    m_frameW[i]     = fw;
    m_frameH[i]     = fh;

    // 扫描每帧 alpha，取所有帧可见像素框的并集（转成帧内相对坐标）作为身体框。
    // 这些图里 200x200 的帧实际只有约 40x53 是角色，底部还有 78px 透明留白，
    // 不处理的话身体框/攻击框/落地判定会全部错位。
    QImage img = sheet.toImage();
    QRect uni;
    for (int k = 0; k < n; ++k) {
        int x0 = k * fw;
        int minX = x0 + fw, minY = fh, maxX = -1, maxY = -1;
        int yLimit = qMin(fh, img.height());
        for (int yy = 0; yy < yLimit; ++yy) {
            const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(yy));
            int xLimit = qMin(x0 + fw, img.width());
            for (int xx = x0; xx < xLimit; ++xx) {
                if (qAlpha(line[xx]) > 8) {
                    if (xx < minX) minX = xx;
                    if (xx > maxX) maxX = xx;
                    if (yy < minY) minY = yy;
                    if (yy > maxY) maxY = yy;
                }
            }
        }
        if (maxX >= 0) {
            QRect bb(minX - x0, minY, maxX - minX + 1, maxY - minY + 1);
            uni = uni.isNull() ? bb : uni.united(bb);
        }
    }
    m_bodyRect[i] = uni.isNull() ? QRect(0, 0, fw, fh) : uni;
    return true;
}

void Fighter::setState(AnimState s) {
    if (m_state == s) return;
    m_state = s;
    m_curFrame = 0;
    m_tick = 0;
}

void Fighter::updateAnimation() {
    int i = static_cast<int>(m_state);
    if (m_frameCount[i] <= 1) return;
    if (++m_tick >= FRAME_DELAY) {
        m_tick = 0;
        m_curFrame++;
        if (m_curFrame >= m_frameCount[i]) {
            // 两组攻击和 Hurt 播完停在末帧，其余循环
            if (m_state == AnimState::Attack || m_state == AnimState::Attack2
                || m_state == AnimState::Hurt)
                m_curFrame = m_frameCount[i] - 1;
            else
                m_curFrame = 0;
        }
    }
}

int Fighter::footGap() const {
    int i = static_cast<int>(m_state);
    if (!m_bodyRect[i].isValid()) return 0;
    int gap = m_frameH[i] - m_bodyRect[i].bottom() - 1;
    return gap > 0 ? gap : 0;
}

void Fighter::draw(QPainter &p) const {
    int i = static_cast<int>(m_state);
    int w = m_frameW[i], h = m_frameH[i];
    // 把整帧下移 footGap，让角色脚底正好落在 y + h 上。
    // 于是 reset(y = GROUND_Y - h) 就能真正贴地，不再悬空。
    QRect target(static_cast<int>(x), static_cast<int>(y + footGap()), w, h);

    if (!m_sheet[i].isNull() && m_frameCount[i] > 0) {
        // 用真实帧宽切图，不再用 width()/frameCount 重算
        int fw = m_frameW[i];
        int fh = m_frameH[i];
        QRect src(m_curFrame * fw, 0, fw, fh);
        if (src.right() > m_sheet[i].width())
            src.setRight(m_sheet[i].width() - 1);

        if (facingRight) {
            p.drawPixmap(target, m_sheet[i], src);
        } else {
            QImage img = m_sheet[i].copy(src).toImage();
            img = img.mirrored(true, false);
            p.drawPixmap(target, QPixmap::fromImage(img));
        }
    } else {
        p.fillRect(target, placeholderColor);
    }

    // 受击闪白：命中瞬间在身体位置叠一层半透明白，打击更有反馈
    if (flashTicks > 0) {
        QRect b = getBodyBox();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 150));
        p.drawEllipse(QPointF(b.center()),
                      static_cast<double>(b.width()) * 0.7,
                      static_cast<double>(b.height()) * 0.55);
        p.setBrush(Qt::NoBrush);
    }
}

int Fighter::getFrameWidth() const {
    return m_frameW[static_cast<int>(m_state)];
}

int Fighter::getFrameCount(AnimState s) const {
    return m_frameCount[static_cast<int>(s)];
}

QRect Fighter::getBodyBox() const {
    int i = static_cast<int>(m_state);
    if (!m_bodyRect[i].isValid())
        return QRect(static_cast<int>(x), static_cast<int>(y), m_frameW[i], m_frameH[i]);
    return QRect(static_cast<int>(x + m_bodyRect[i].left()),
                 static_cast<int>(y + footGap() + m_bodyRect[i].top()),
                 m_bodyRect[i].width(), m_bodyRect[i].height());
}

QRect Fighter::getHitBox() const {
    if (!isAttackActive()) return QRect();
    QRect b = getBodyBox();
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
    m_state = AnimState::Idle; m_curFrame = 0; m_tick = 0;
}

void Fighter::faceTo(const Fighter &other) {
    if (other.x == x) return;   // 重合时保持原朝向，避免两人同时朝右
    facingRight = other.x > x;
}
