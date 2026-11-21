#pragma once

#include <QObject>

// Tracks KWin's view of whether a text field is focused, via the `active`
// property of org.kde.kwin.VirtualKeyboard (true while the focused app has
// text input enabled on an editable). KWin cannot show fcitx's virtual
// keyboard itself (fcitx renders through skvirt over D-Bus, not through a
// KWin input-panel surface), but it does reliably report *when* there is a
// field to type into — exactly the signal skvirt needs for auto show/hide.
class KWinVk : public QObject
{
    Q_OBJECT
public:
    explicit KWinVk(QObject *parent = nullptr);

    bool textInputFocused() const { return m_focused; }

    // Output (connector name, e.g. "eDP-1") KWin maps the evdev touch/tablet
    // device sysName ("event11") to; empty when it is left unmapped (KWin
    // then picks the internal panel itself) or KWin can't be asked.
    QString deviceOutputName(const QString &sysName) const;
    // KWin's active output — the one under the pointer with default
    // settings; empty if KWin can't be asked.
    QString activeOutputName() const;

signals:
    void textInputFocusChanged(bool focused);

private slots:
    void refresh();

private:
    bool m_focused = false;
};
