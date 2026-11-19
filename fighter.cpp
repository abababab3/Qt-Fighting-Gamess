#include "fighter.h"

Fighter::Fighter() {}

bool Fighter::loadFrames(AnimState state, const QString &path,
                         int frameCount, int fw, int fh) {
    QPixmap sheet(path);
    if (sheet.isNull()) return false;
    int i = static_cast<int>(state);
    m_sheet[i]      = sheet;
    m_frameCount[i] = frameCount;
    m_frameW[i]     = fw;
    m_frameH[i]     = fh;
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
            // Attack/Hurt 播完停在末帧，其余循环
            if (m_state == AnimState::Attack || m_state == AnimState::Hurt)
                m_curFrame = m_frameCount[i] - 1;
            else
                m_curFrame = 0;
        }
    }
}

void Fighter::draw(QPainter &p) const {
    int i = static_cast<int>(m_state);
    int w = m_frameW[i], h = m_frameH[i];
    QRect target(static_cast<int>(x), static_cast<int>(y), w, h);

    if (!m_sheet[i].isNull() && m_frameCount[i] > 0) {
        int fw = m_sheet[i].width() / m_frameCount[i];
        int fh = m_sheet[i].height();
        QRect src(m_curFrame * fw, 0, fw, fh);

        if (facingRight) {
            p.drawPixmap(target, m_sheet[i], src);
        } else {
            // 兼容旧版 Qt 的镜像翻转写法
            QImage img = m_sheet[i].copy(src).toImage();
            img = img.mirrored(true, false);
            p.drawPixmap(target, QPixmap::fromImage(img));
        }
    } else {
        p.fillRect(target, placeholderColor);
    }
}

QRect Fighter::getBodyBox() const {
    int i = static_cast<int>(m_state);
    return QRect(static_cast<int>(x), static_cast<int>(y), m_frameW[i], m_frameH[i]);
}

QRect Fighter::getHitBox() const {
    if (!isAttackActive()) return QRect();
    QRect b = getBodyBox();
    int reach = 45;
    if (facingRight) return QRect(b.right(), b.top() + b.height()/4, reach, b.height()/2);
    return QRect(b.left() - reach, b.top() + b.height()/4, reach, b.height()/2);
}

bool Fighter::isAttackActive() const {
    if (m_state != AnimState::Attack) return false;
    int i = static_cast<int>(m_state);
    if (m_frameCount[i] <= 1) return true;
    return m_curFrame > m_frameCount[i]/3 && m_curFrame < m_frameCount[i]*2/3;
}

void Fighter::reset(double startX, double startY) {
    x = startX; y = startY; vx = 0; vy = 0;
    hp = maxHp; onGround = false;
    m_state = AnimState::Idle; m_curFrame = 0; m_tick = 0;
}

void Fighter::faceTo(const Fighter &other) {
    facingRight = other.x >= x;
}