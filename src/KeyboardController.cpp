#include "KeyboardController.h"
#include "UinputKeyboard.h"
#include "InputMonitor.h"
#include "KWinVk.h"
#include "FcitxIm.h"

#include <QHash>
#include <QDebug>

// Physical key info: evdev keycode and whether Shift is needed to produce the
// character on a US layout. We inject a hardware key via uinput, so fcitx's
// active engine (keyboard layout, pinyin, …) processes it just like real input.
struct PhysKey { int keycode; bool shift; };

// Map a printable character to the US-layout physical (evdev) key that
// produces it.
static bool physKeyForChar(QChar ch, PhysKey &out)
{
    static const QHash<QChar, PhysKey> table = []{
        QHash<QChar, PhysKey> t;
        auto add = [&](const char *chars, std::initializer_list<int> codes, bool shift) {
            int i = 0;
            for (int code : codes) t[QChar(chars[i++])] = {code, shift};
        };
        // Same as add(), but for non-ASCII alphabets that can't round-trip
        // through a `const char*` literal.
        auto addU = [&](const QString &chars, std::initializer_list<int> codes, bool shift) {
            int i = 0;
            for (int code : codes) t[chars.at(i++)] = {code, shift};
        };
        // Letters: lowercase (no shift) and uppercase (shift), same evdev codes.
        const int lcode[] = {
            30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
        for (int i = 0; i < 26; ++i) {
            t[QChar('a' + i)] = {lcode[i], false};
            t[QChar('A' + i)] = {lcode[i], true};
        }
        // Digit row, unshifted and shifted symbols (share evdev codes 2..11).
        add("1234567890", {2,3,4,5,6,7,8,9,10,11}, false);
        add("!@#$%^&*()", {2,3,4,5,6,7,8,9,10,11}, true);
        // Other punctuation (unshifted / shifted) on their keys.
        add("-=[]\\;'`,./", {12,13,26,27,43,39,40,41,51,52,53}, false);
        add("_+{}|:\"~<>?", {12,13,26,27,43,39,40,41,51,52,53}, true);
        // Russian (ЙЦУКЕН): same evdev codes as the Latin keys in the same
        // physical positions, since that's the standard Cyrillic mapping.
        addU(QStringLiteral("йцукенгшщзхъ"), {16,17,18,19,20,21,22,23,24,25,26,27}, false);
        addU(QStringLiteral("ЙЦУКЕНГШЩЗХЪ"), {16,17,18,19,20,21,22,23,24,25,26,27}, true);
        addU(QStringLiteral("фывапролдже"), {30,31,32,33,34,35,36,37,38,39,40}, false);
        addU(QStringLiteral("ФЫВАПРОЛДЖЭ"), {30,31,32,33,34,35,36,37,38,39,40}, true);
        addU(QStringLiteral("ячсмитьбю"), {44,45,46,47,48,49,50,51,52}, false);
        addU(QStringLiteral("ЯЧСМИТЬБЮ"), {44,45,46,47,48,49,50,51,52}, true);
        t[QChar(u'ё')] = {41, false};
        t[QChar(u'Ё')] = {41, true};
        return t;
    }();
    auto it = table.constFind(ch);
    if (it == table.constEnd())
        return false;
    out = *it;
    return true;
}

// Named control keys -> evdev keycode.
static bool specialKey(const QString &name, int &keycode)
{
    static const QHash<QString, int> t = {
        {"space", 57},  {"backspace", 14}, {"tab", 15},   {"enter", 28},
        {"escape", 1},  {"delete", 111},   {"left", 105}, {"up", 103},
        {"right", 106}, {"down", 108},
    };
    auto it = t.constFind(name.toLower());
    if (it == t.constEnd())
        return false;
    keycode = *it;
    return true;
}

KeyboardController::KeyboardController(QObject *parent) : QObject(parent)
{
    QMetaObject::invokeMethod(this, &KeyboardController::initBackend,
                              Qt::QueuedConnection);
}

KeyboardController::~KeyboardController() = default;

void KeyboardController::initBackend()
{
    m_kbd   = std::make_unique<UinputKeyboard>();
    m_input = std::make_unique<InputMonitor>();
    m_kwin  = std::make_unique<KWinVk>();
    m_fcitx = std::make_unique<FcitxIm>();

    // Reflect whatever IM fcitx5 is already active on, rather than assuming
    // English.
    m_layout = m_fcitx->currentIM();
    m_layoutLabel = m_fcitx->shortLabel(m_layout);
    emit layoutChanged();

    // skvirt owns its own visibility policy (touch → show, mouse/lost focus →
    // hide) and injects keys as a plain virtual keyboard. fcitx is never told
    // anything, so it stays in its normal mode (tray + candidate window).
    connect(m_input.get(), &InputMonitor::touchActivity,
            this, &KeyboardController::onTouchActivity);
    connect(m_input.get(), &InputMonitor::pointerActivity,
            this, &KeyboardController::onPointerActivity);
    connect(m_kwin.get(), &KWinVk::textInputFocusChanged,
            this, &KeyboardController::onFieldFocusChanged);

    m_fieldFocused = m_kwin->textInputFocused();
}

void KeyboardController::setVisible(bool v)
{
    if (m_visible == v)
        return;
    m_visible = v;
    emit panelVisibleChanged();
}

// ---- auto show/hide engine ----------------------------------------------

void KeyboardController::showKeyboard()
{
    if (m_visible)
        return;
    qDebug() << "skvirt: SHOW (touch + field focused)";
    setVisible(true);
}

void KeyboardController::hideKeyboard()
{
    if (!m_visible)
        return;
    qDebug() << "skvirt: HIDE";
    setVisible(false);
}

void KeyboardController::onTouchActivity()
{
    // Finger/pen: enter touch mode and pop the keyboard if there's a field.
    m_touchMode = true;
    if (m_fieldFocused)
        showKeyboard();
}

void KeyboardController::onPointerActivity()
{
    // The user reached for the mouse/touchpad — get out of the way, like Windows.
    m_touchMode = false;
    hideKeyboard();
}

void KeyboardController::onFieldFocusChanged(bool focused)
{
    m_fieldFocused = focused;
    if (!focused)
        hideKeyboard();           // field/app went away
    else if (m_touchMode)
        showKeyboard();           // tapped into a field
}

void KeyboardController::commitText(const QString &text)
{
    if (text.isEmpty() || !m_kbd)
        return;

    QChar ch = text.at(0);
    PhysKey pk;
    if (!physKeyForChar(ch, pk)) {
        qWarning() << "skvirt: no physical key mapping for" << ch;
        return;
    }
    // Inject the US-position key; the active fcitx layout/engine decides the
    // actual output (latin, cyrillic, pinyin composition, …).
    m_kbd->tap(pk.keycode, pk.shift);

    if (m_shift) {  // one-shot shift
        m_shift = false;
        emit shiftActiveChanged();
    }
}

void KeyboardController::sendSpecial(const QString &name)
{
    if (!m_kbd)
        return;
    int keycode;
    if (!specialKey(name, keycode)) {
        qWarning() << "skvirt: unknown special key:" << name;
        return;
    }
    m_kbd->tap(keycode, false);
}

void KeyboardController::toggleShift()
{
    m_shift = !m_shift;
    emit shiftActiveChanged();
}

void KeyboardController::toggleCapsLock()
{
    m_capsLock = !m_capsLock;
    m_shift = false;
    emit capsLockChanged();
    emit shiftActiveChanged();
}

void KeyboardController::toggleSymbolMode()
{
    m_symbolMode = !m_symbolMode;
    emit symbolModeChanged();
}

void KeyboardController::cycleLayout()
{
    if (!m_fcitx)
        return;
    const QStringList ims = m_fcitx->groupInputMethods();
    if (ims.isEmpty())
        return;
    const int idx = ims.indexOf(m_layout);
    const QString next = ims.at((idx + 1) % ims.size());
    m_fcitx->setCurrentIM(next);
    m_layout = next;
    m_layoutLabel = m_fcitx->shortLabel(next);
    emit layoutChanged();
}

void KeyboardController::hidePanel()
{
    // Manual dismiss (▼): drop touch mode so it doesn't immediately reappear
    // while the same field keeps focus — the user asks again by tapping it.
    m_touchMode = false;
    hideKeyboard();
}
