#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QString>

// A kernel virtual keyboard via /dev/uinput. Key taps injected here enter the
// normal input stack (libinput → KWin → fcitx) exactly like a physical
// keyboard, so the active fcitx layout/engine (keyboard-ru, pinyin, …) composes
// them and shows candidates in its usual classicui window. fcitx is never put
// into its "virtual keyboard" mode, so its tray icon and candidate popups keep
// working and nothing can get stuck.
//
// Requires write access to /dev/uinput (user in the "input" group).
class UinputKeyboard : public QObject
{
    Q_OBJECT
public:
    explicit UinputKeyboard(QObject *parent = nullptr);
    ~UinputKeyboard() override;

    bool ready() const { return m_fd >= 0; }

    // Press+release one evdev keycode, optionally with Shift and any further
    // modifier keycodes (Ctrl/Alt/Meta) held around it.
    void tap(int keycode, bool shift, const QList<int> &held = {});

    // Type arbitrary Unicode text through fcitx5's unicode addon direct mode:
    // for each character emit the Ctrl+Shift+U chord, the hex codepoint
    // digits, then Enter. Works regardless of the active layout/engine.
    void typeUnicode(const QString &text);

private:
    void emitEvent(int type, int code, int value);
    // Called before writing `upcoming` events: pauses a long burst so the
    // reader can drain the kernel's small per-reader buffer (see the .cpp).
    void pace(int upcoming);

    int m_fd = -1;
    int m_queued = 0;             // events written since the reader last had time to drain
    QElapsedTimer m_sinceWrite;   // time since the last event write
};
