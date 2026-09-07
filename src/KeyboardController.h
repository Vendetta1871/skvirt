#pragma once

#include "LayoutGenerator.h"

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <memory>

class UinputKeyboard;
class InputMonitor;
class KWinVk;
class FcitxIm;
class SuggestionEngine;

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
    // Latched Mac modifiers (⌃ ⌥ ⌘): armed by a tap, held around the next
    // key and released again, exactly like the one-shot Shift.
    Q_PROPERTY(bool controlActive READ controlActive NOTIFY modifiersChanged)
    Q_PROPERTY(bool optionActive READ optionActive NOTIFY modifiersChanged)
    Q_PROPERTY(bool commandActive READ commandActive NOTIFY modifiersChanged)
    Q_PROPERTY(QString layout READ layout NOTIFY layoutChanged)
    Q_PROPERTY(QString layoutLabel READ layoutLabel NOTIFY layoutChanged)
    Q_PROPERTY(QStringList suggestions READ suggestions NOTIFY suggestionsChanged)
    // Typing rows generated from the active IM's xkb layout (4 rows of
    // [keyName, label, shiftLabel, width]: number row + the three letter
    // rows); empty when the active IM is not a keyboard-* IM or generation
    // failed — QML falls back to hardcoded rows.
    Q_PROPERTY(QVariantList generatedRows READ generatedRows NOTIFY layoutChanged)

public:
    explicit KeyboardController(QObject *parent = nullptr);
    ~KeyboardController() override;

    bool panelVisible() const { return m_visible; }
    bool shiftActive() const { return m_shift; }
    bool capsLock() const { return m_capsLock; }
    bool symbolMode() const { return m_symbolMode; }
    bool controlActive() const { return m_control; }
    bool optionActive() const { return m_option; }
    bool commandActive() const { return m_command; }
    QString layout() const { return m_layout; }
    QString layoutLabel() const { return m_layoutLabel; }
    QStringList suggestions() const { return m_suggestions; }
    QVariantList generatedRows() const { return m_generatedRows; }

    // Printable key: send the resolved character (case/symbol already applied in QML).
    Q_INVOKABLE void commitText(const QString &text);
    // Positional key from a generated layout: keyName is the US-position name
    // ("q", "[", ";", …), shifted is the effective shift state, producedChar
    // is the label QML displayed (used only to keep the word buffer in sync).
    Q_INVOKABLE void commitKeyAt(const QString &keyName, bool shifted, const QString &producedChar);
    // Named control key: backspace/enter/tab/escape/delete/space/up/down/left/right.
    Q_INVOKABLE void sendSpecial(const QString &name);
    Q_INVOKABLE void toggleShift();
    // Arm/disarm one latched modifier: "control", "option" or "command".
    Q_INVOKABLE void toggleModifier(const QString &name);
    Q_INVOKABLE void toggleCapsLock();
    Q_INVOKABLE void toggleSymbolMode();
    // Switch fcitx5 to the next input method configured in the current group.
    Q_INVOKABLE void cycleLayout();
    // Accept suggestion #index from the bar: completes the word (hunspell
    // backend) or commits the hanzi candidate (pinyin backend).
    Q_INVOKABLE void commitSuggestion(int index);
    // Manual hide: drops touch mode so the panel doesn't immediately reappear.
    Q_INVOKABLE void hidePanel();
    // Manual toggle (tray icon): shows the panel even when the auto engine
    // wouldn't (no touch activity, tablet-mode gate), hides it otherwise.
    Q_INVOKABLE void togglePanel();

signals:
    void panelVisibleChanged();
    void shiftActiveChanged();
    void capsLockChanged();
    void symbolModeChanged();
    void modifiersChanged();
    void layoutChanged();
    void suggestionsChanged();

private:
    void initBackend();
    // Evdev keycodes of the currently latched modifiers, and their release
    // after a key has been sent with them.
    QList<int> heldModifiers() const;
    void clearModifiers();
    void setVisible(bool v);
    // Regenerate m_generatedRows for the active IM (keyboard-* only).
    void regenerateLayout();

    // Current-word buffer feeding the SuggestionEngine.
    void updateWordBuffer(QChar committed);
    void refreshSuggestions();

    // Auto show/hide decision engine (Windows-like touch behaviour).
    void showKeyboard();   // map + ensure fcitx VK backend loaded for key routing
    void hideKeyboard();   // unmap + tell fcitx
    void onTouchActivity();
    void onPointerActivity();
    void onTabletModeChanged(bool tabletMode);
    void onFieldFocusChanged(bool focused);

    std::unique_ptr<UinputKeyboard> m_kbd;
    std::unique_ptr<InputMonitor> m_input;
    std::unique_ptr<KWinVk> m_kwin;
    std::unique_ptr<FcitxIm> m_fcitx;
    std::unique_ptr<SuggestionEngine> m_engine;

    LayoutGenerator m_layoutGen;
    QVariantList m_generatedRows;

    QString m_word;              // current-word buffer (capped, see updateWordBuffer)
    QStringList m_suggestions;

    bool m_visible = false;
    bool m_shift = false;
    bool m_capsLock = false;
    bool m_symbolMode = false;
    bool m_control = false;   // ⌃ latched
    bool m_option = false;    // ⌥ latched
    bool m_command = false;   // ⌘ latched
    QString m_layout;       // active IM unique name, e.g. "keyboard-ru"
    QString m_layoutLabel;  // its short display code, e.g. "ru"

    // Engine state.
    bool m_touchMode = false;    // most recent interaction was a finger/pen, not a mouse
    bool m_fieldFocused = false; // KWin reports an editable client is focused
    bool m_tabletMode = false;   // SW_TABLET_MODE switch position from InputMonitor
};
