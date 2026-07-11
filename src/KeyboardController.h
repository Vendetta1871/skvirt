#pragma once

#include <QObject>
#include <QQmlEngine>
#include <memory>

class UinputKeyboard;
class InputMonitor;
class KWinVk;
class FcitxIm;

// QML singleton — turns key taps into fcitx5 key events over D-Bus.
class KeyboardController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool panelVisible READ panelVisible NOTIFY panelVisibleChanged)
    Q_PROPERTY(bool shiftActive READ shiftActive NOTIFY shiftActiveChanged)
    Q_PROPERTY(bool capsLock READ capsLock NOTIFY capsLockChanged)
    Q_PROPERTY(bool symbolMode READ symbolMode NOTIFY symbolModeChanged)
    Q_PROPERTY(QString layout READ layout NOTIFY layoutChanged)
    Q_PROPERTY(QString layoutLabel READ layoutLabel NOTIFY layoutChanged)

public:
    explicit KeyboardController(QObject *parent = nullptr);
    ~KeyboardController() override;

    bool panelVisible() const { return m_visible; }
    bool shiftActive() const { return m_shift; }
    bool capsLock() const { return m_capsLock; }
    bool symbolMode() const { return m_symbolMode; }
    QString layout() const { return m_layout; }
    QString layoutLabel() const { return m_layoutLabel; }

    // Printable key: send the resolved character (case/symbol already applied in QML).
    Q_INVOKABLE void commitText(const QString &text);
    // Named control key: backspace/enter/tab/escape/delete/space/up/down/left/right.
    Q_INVOKABLE void sendSpecial(const QString &name);
    Q_INVOKABLE void toggleShift();
    Q_INVOKABLE void toggleCapsLock();
    Q_INVOKABLE void toggleSymbolMode();
    // Switch fcitx5 to the next input method configured in the current group.
    Q_INVOKABLE void cycleLayout();
    // Manual hide (▼ button): also tells fcitx5.
    Q_INVOKABLE void hidePanel();

signals:
    void panelVisibleChanged();
    void shiftActiveChanged();
    void capsLockChanged();
    void symbolModeChanged();
    void layoutChanged();

private:
    void initBackend();
    void setVisible(bool v);

    // Auto show/hide decision engine (Windows-like touch behaviour).
    void showKeyboard();   // map + ensure fcitx VK backend loaded for key routing
    void hideKeyboard();   // unmap + tell fcitx
    void onTouchActivity();
    void onPointerActivity();
    void onFieldFocusChanged(bool focused);

    std::unique_ptr<UinputKeyboard> m_kbd;
    std::unique_ptr<InputMonitor> m_input;
    std::unique_ptr<KWinVk> m_kwin;
    std::unique_ptr<FcitxIm> m_fcitx;

    bool m_visible = false;
    bool m_shift = false;
    bool m_capsLock = false;
    bool m_symbolMode = false;
    QString m_layout;       // active IM unique name, e.g. "keyboard-ru"
    QString m_layoutLabel;  // its short display code, e.g. "ru"

    // Engine state.
    bool m_touchMode = false;    // most recent interaction was a finger/pen, not a mouse
    bool m_fieldFocused = false; // KWin reports an editable client is focused
};
