#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <QString>
#include <array>

enum class ControlAction { Left, Right, Jump, FastFall, Attack, Guard, Skill, Count };

class KeyBindings {
public:
    static constexpr int PlayerCount = 2;
    static constexpr int ActionCount = static_cast<int>(ControlAction::Count);

    KeyBindings();

    int key(int player, ControlAction action) const;
    ControlAction actionForKey(int player, int key) const;
    QString rebind(int player, ControlAction action, int key);
    void restoreDefaults();

    static QString actionName(ControlAction action);
    static QString keyName(int key);

private:
    using Keys = std::array<int, ActionCount * PlayerCount>;
    static const Keys Defaults;
    static bool isBindable(int key);
    void save() const;

    Keys m_keys = Defaults;
};

#endif // KEYBINDINGS_H
