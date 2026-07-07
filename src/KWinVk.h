#pragma once

#include <QObject>

// Tracks KWin's view of whether the currently focused client accepts text
// input, via org.kde.kwin.VirtualKeyboard. KWin cannot show fcitx's virtual
// keyboard itself (fcitx renders through skvirt over D-Bus, not through a
// KWin input-panel surface), but it does reliably report *when* an editable
// client is focused — which is exactly the "there is a field to type into"
// signal skvirt needs to decide auto show/hide.
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
