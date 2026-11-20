#include "keybindings.h"

#include <QKeySequence>
#include <QSettings>
#include <QSet>
#include <QVariant>
#include <algorithm>

namespace {

const auto SettingsKey = QStringLiteral("input/playerBindings");

QSettings settings()
{
    return QSettings(QSettings::defaultFormat(), QSettings::UserScope,
                     QStringLiteral("QtFightingGames"), QStringLiteral("QtFightingGames"));
}

int keyIndex(int player, ControlAction action)
{
    return player * KeyBindings::ActionCount + static_cast<int>(action);
}

} // namespace

const KeyBindings::Keys KeyBindings::Defaults{
    Qt::Key_A, Qt::Key_D, Qt::Key_W, Qt::Key_S, Qt::Key_J, Qt::Key_K, Qt::Key_L,
    Qt::Key_Left, Qt::Key_Right, Qt::Key_Up, Qt::Key_Down, Qt::Key_1, Qt::Key_2, Qt::Key_3
};

KeyBindings::KeyBindings()
{
    const QVariantList stored = settings().value(SettingsKey).toList();
    if (stored.size() != static_cast<qsizetype>(m_keys.size())) return;

    Keys parsed{};
    QSet<int> occupied;
    for (int i = 0; i < stored.size(); ++i) {
        bool valid = false;
        const int key = stored.at(i).toInt(&valid);
        if (!valid || !isBindable(key) || occupied.contains(key)) return;
        occupied.insert(key);
        parsed[i] = key;
    }
    m_keys = parsed;
}

int KeyBindings::key(int player, ControlAction action) const
{
    return m_keys[keyIndex(player, action)];
}

ControlAction KeyBindings::actionForKey(int player, int pressedKey) const
{
    for (int index = 0; index < ActionCount; ++index) {
        const auto action = static_cast<ControlAction>(index);
        if (key(player, action) == pressedKey) return action;
    }
    return ControlAction::Count;
}

QString KeyBindings::rebind(int player, ControlAction action, int newKey)
{
    if (!isBindable(newKey)) return QStringLiteral("这个按键用于系统操作，请换一个键。");
    const int index = keyIndex(player, action);
    if (m_keys[index] == newKey) return {};

    const auto collision = std::find(m_keys.cbegin(), m_keys.cend(), newKey);
    if (collision != m_keys.cend()) {
        const int existing = static_cast<int>(collision - m_keys.cbegin());
        return QStringLiteral("%1 已用于 %2P「%3」，请换一个键。")
            .arg(keyName(newKey)).arg(existing / ActionCount + 1)
            .arg(actionName(static_cast<ControlAction>(existing % ActionCount)));
    }

    m_keys[index] = newKey;
    save();
    return {};
}

void KeyBindings::restoreDefaults()
{
    m_keys = Defaults;
    save();
}

QString KeyBindings::actionName(ControlAction action)
{
    switch (action) {
    case ControlAction::Left: return QStringLiteral("向左移动");
    case ControlAction::Right: return QStringLiteral("向右移动");
    case ControlAction::Jump: return QStringLiteral("跳跃");
    case ControlAction::FastFall: return QStringLiteral("快速下落");
    case ControlAction::Attack: return QStringLiteral("普通攻击");
    case ControlAction::Guard: return QStringLiteral("格挡（按住）");
    case ControlAction::Skill: return QStringLiteral("角色技能");
    default: return {};
    }
}

QString KeyBindings::keyName(int key)
{
    switch (key) {
    case Qt::Key_Left: return QStringLiteral("←");
    case Qt::Key_Right: return QStringLiteral("→");
    case Qt::Key_Up: return QStringLiteral("↑");
    case Qt::Key_Down: return QStringLiteral("↓");
    default: return QKeySequence(key).toString(QKeySequence::NativeText);
    }
}

bool KeyBindings::isBindable(int key)
{
    switch (key) {
    case Qt::Key_Space:
    case Qt::Key_Escape:
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_M:
    case Qt::Key_R:
    case Qt::Key_T:
    case Qt::Key_F12:
        return false;
    default:
        return (key >= Qt::Key_Exclam && key <= Qt::Key_AsciiTilde)
               || (key >= Qt::Key_Left && key <= Qt::Key_Down)
               || (key >= Qt::Key_F1 && key <= Qt::Key_F11);
    }
}

void KeyBindings::save() const
{
    QVariantList stored;
    for (int key : m_keys) stored.append(key);
    settings().setValue(SettingsKey, stored);
}
