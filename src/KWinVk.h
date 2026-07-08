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

signals:
    void textInputFocusChanged(bool focused);

private slots:
    void refresh();

private:
    bool m_focused = false;
};
