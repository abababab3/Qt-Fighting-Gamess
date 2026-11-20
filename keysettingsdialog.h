#ifndef KEYSETTINGSDIALOG_H
#define KEYSETTINGSDIALOG_H

#include "keybindings.h"
#include <QDialog>

class QLabel;
class QPushButton;

class KeySettingsDialog final : public QDialog {
public:
    explicit KeySettingsDialog(KeyBindings &bindings, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void beginCapture(int row, int player);
    void refreshKeys();
    void setMessage(const QString &message, bool error = false);

    KeyBindings &m_bindings;
    std::array<std::array<QPushButton *, KeyBindings::PlayerCount>, KeyBindings::ActionCount + 1> m_buttons{};
    QLabel *m_message;
    int m_captureRow = -1;
    int m_capturePlayer = 0;
};

#endif // KEYSETTINGSDIALOG_H
