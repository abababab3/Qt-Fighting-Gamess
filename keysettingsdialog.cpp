#include "keysettingsdialog.h"

#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>

namespace {

constexpr int Rows = KeyBindings::ActionCount + 1;

} // namespace

KeySettingsDialog::KeySettingsDialog(KeyBindings &bindings, QWidget *parent)
    : QDialog(parent), m_bindings(bindings), m_message(new QLabel(this))
{
    setWindowTitle(QStringLiteral("键位设置"));
    setMinimumWidth(460);
    setStyleSheet(QStringLiteral(
        "QDialog { background: #211c28; }"
        "QLabel { color: #f6ebda; font-size: 14px; }"
        "QLabel#title { font-size: 25px; font-weight: bold; }"
        "QLabel#note { color: #c0afa9; font-size: 12px; }"
        "QPushButton { color: #f6ebda; background: #302834; border: 1px solid #70585b;"
        " border-radius: 5px; padding: 5px 10px; font-size: 14px; }"
        "QPushButton:hover { background: #453541; }"
        "QPushButton:focus { border: 2px solid #a9e0df; }"));

    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(24, 18, 24, 20);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(7);
    grid->setColumnStretch(0, 2);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);

    auto *title = new QLabel(QStringLiteral("键位设置"), this);
    title->setObjectName(QStringLiteral("title"));
    grid->addWidget(title, 0, 0, 1, 3);
    auto *note = new QLabel(QStringLiteral("选中一个键位并按空格，或点击它，然后按下新键。"), this);
    note->setObjectName(QStringLiteral("note"));
    grid->addWidget(note, 1, 0, 1, 3);
    grid->addWidget(new QLabel(QStringLiteral("玩家 1"), this), 2, 1, Qt::AlignCenter);
    grid->addWidget(new QLabel(QStringLiteral("玩家 2"), this), 2, 2, Qt::AlignCenter);

    for (int row = 0; row < KeyBindings::ActionCount; ++row) {
        const auto action = static_cast<ControlAction>(row);
        const QString name = KeyBindings::actionName(action);
        grid->addWidget(new QLabel(name, this), row + 3, 0);
        for (int player = 0; player < KeyBindings::PlayerCount; ++player) {
            auto *button = new QPushButton(this);
            button->setAutoDefault(false);
            button->setAccessibleName(QStringLiteral("%1P %2").arg(player + 1).arg(name));
            m_buttons[row][player] = button;
            button->installEventFilter(this);
            connect(button, &QPushButton::clicked, this, [this, row, player] { beginCapture(row, player); });
            grid->addWidget(button, row + 3, player + 1);
        }
    }

    m_message->setMinimumHeight(30);
    m_message->setWordWrap(true);
    grid->addWidget(m_message, Rows + 2, 0, 1, 3);
    auto *fixedKeys = new QLabel(QStringLiteral("瞬身：双击移动键后按普攻或技能。用强化技能会延长瞬身冷却。\n防反：命中前按格挡，成功后按普攻。\n"
                                               "青篁：短按技能借风，长按技能纳风，松手释放。\n"
                                               "固定快捷键：Esc 返回 · M 静音 · F12 判定框 · R 重置练习 · T 切换木桩"), this);
    fixedKeys->setObjectName(QStringLiteral("note"));
    fixedKeys->setWordWrap(true);
    grid->addWidget(fixedKeys, Rows + 3, 0, 1, 3);

    auto *restore = new QPushButton(QStringLiteral("恢复默认"), this);
    auto *back = new QPushButton(QStringLiteral("返回主菜单"), this);
    m_buttons.back() = {restore, back};
    for (auto *button : m_buttons.back()) {
        button->setAutoDefault(false);
        button->installEventFilter(this);
    }
    grid->addWidget(restore, Rows + 4, 1);
    grid->addWidget(back, Rows + 4, 2);
    connect(restore, &QPushButton::clicked, this, [this] {
        m_captureRow = -1;
        m_bindings.restoreDefaults();
        refreshKeys();
        setMessage(QStringLiteral("双方键位已恢复默认。"));
    });
    connect(back, &QPushButton::clicked, this, &QDialog::accept);
    refreshKeys();
    m_buttons.front().front()->setFocus();
}

void KeySettingsDialog::beginCapture(int row, int player)
{
    m_captureRow = row;
    m_capturePlayer = player;
    refreshKeys();
    m_buttons[row][player]->setText(QStringLiteral("按下新键…"));
    m_buttons[row][player]->setFocus();
    setMessage(QStringLiteral("正在修改 %1P「%2」。按 Esc 取消。")
                   .arg(player + 1).arg(KeyBindings::actionName(static_cast<ControlAction>(row))));
}

void KeySettingsDialog::refreshKeys()
{
    for (int row = 0; row < KeyBindings::ActionCount; ++row) {
        const auto action = static_cast<ControlAction>(row);
        for (int player = 0; player < KeyBindings::PlayerCount; ++player)
            m_buttons[row][player]->setText(KeyBindings::keyName(m_bindings.key(player, action)));
    }
}

void KeySettingsDialog::setMessage(const QString &message, bool error)
{
    m_message->setStyleSheet(error ? QStringLiteral("color: #ffa38e;") : QStringLiteral("color: #a9e0df;"));
    m_message->setText(message);
}

bool KeySettingsDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (m_captureRow >= 0 && event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    if (event->type() != QEvent::KeyPress) return QDialog::eventFilter(watched, event);
    auto *keyEvent = static_cast<QKeyEvent *>(event);

    if (m_captureRow >= 0) {
        if (keyEvent->isAutoRepeat()) return true;
        if (keyEvent->key() == Qt::Key_Escape) {
            m_captureRow = -1;
            refreshKeys();
            setMessage(QStringLiteral("已取消修改。"));
            return true;
        }
        if (keyEvent->modifiers() & ~Qt::KeypadModifier) {
            setMessage(QStringLiteral("请按一个单键。"), true);
            return true;
        }

        const auto action = static_cast<ControlAction>(m_captureRow);
        const QString error = m_bindings.rebind(m_capturePlayer, action, keyEvent->key());
        if (!error.isEmpty()) {
            setMessage(error, true);
            return true;
        }

        m_captureRow = -1;
        refreshKeys();
        setMessage(QStringLiteral("键位已保存。"));
        return true;
    }

    int rowOffset = 0;
    bool switchColumn = false;
    switch (keyEvent->key()) {
    case Qt::Key_Up: rowOffset = -1; break;
    case Qt::Key_Down: rowOffset = 1; break;
    case Qt::Key_Left:
    case Qt::Key_Right: switchColumn = true; break;
    default: return QDialog::eventFilter(watched, event);
    }
    for (int row = 0; row < Rows; ++row) {
        for (int player = 0; player < KeyBindings::PlayerCount; ++player) {
            if (m_buttons[row][player] != watched) continue;
            m_buttons[(row + rowOffset + Rows) % Rows][switchColumn ? 1 - player : player]->setFocus();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
