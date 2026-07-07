#pragma once

#include <QObject>
#include <QVector>

class QSocketNotifier;

// Watches raw evdev input devices so skvirt can tell *how* the user is
// interacting: a finger/pen on the touchscreen (→ show the keyboard, like
// Windows) versus a mouse/touchpad/trackpoint (→ hide it). KWin does not
// expose this distinction to clients, and its touch-driven OSK logic cannot
// reach fcitx's virtual-keyboard mode, so we read the devices directly.
//
// Requires read access to /dev/input/event* (the user must be in the "input"
// group). Devices that cannot be opened are silently skipped.
class InputMonitor : public QObject
{
    Q_OBJECT
public:
    explicit InputMonitor(QObject *parent = nullptr);
    ~InputMonitor() override;

    bool hasTouchscreen() const { return m_hasTouch; }

signals:
    void touchActivity();    // finger/pen went down on a direct (touchscreen) device
    void pointerActivity();  // mouse/touchpad/trackpoint moved or clicked

private:
    enum Kind { Ignore, Touch, Pointer };
    struct Dev { int fd; Kind kind; QSocketNotifier *notifier; };

    void openDevices();
    void onReadable(int fd, Kind kind);

    QVector<Dev> m_devs;
    bool m_hasTouch = false;
};
