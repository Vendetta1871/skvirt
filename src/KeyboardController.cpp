#include "KeyboardController.h"
#include "UinputKeyboard.h"
#include "InputMonitor.h"
#include "KWinVk.h"
#include "FcitxIm.h"
#include "SuggestionEngine.h"

#include "skvirt.h"  // generated SkvirtSettings (kconfig_add_kcfg_files)

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
        {"right", 106}, {"down", 108},     {"capslock", 58},
        // Function row (KEY_F1..KEY_F10 are contiguous, F11/F12 are not).
        {"f1", 59},  {"f2", 60},  {"f3", 61},  {"f4", 62},  {"f5", 63},
        {"f6", 64},  {"f7", 65},  {"f8", 66},  {"f9", 67},  {"f10", 68},
        {"f11", 87}, {"f12", 88},
    };
    auto it = t.constFind(name.toLower());
    if (it == t.constEnd())
        return false;
    keycode = *it;
    return true;
}

// US-position key name (as used in the QML row data and by LayoutGenerator)
// -> evdev keycode. Same codes as physKeyForChar's letter/punctuation rows.
static bool evdevForKeyName(const QString &name, int &keycode)
{
    static const QHash<QString, int> t = {
        {"`", 41}, {"1", 2},  {"2", 3},  {"3", 4},  {"4", 5},  {"5", 6},
        {"6", 7},  {"7", 8},  {"8", 9},  {"9", 10}, {"0", 11}, {"-", 12},
        {"=", 13},
        {"q", 16}, {"w", 17}, {"e", 18}, {"r", 19}, {"t", 20}, {"y", 21},
        {"u", 22}, {"i", 23}, {"o", 24}, {"p", 25}, {"[", 26}, {"]", 27},
        {"a", 30}, {"s", 31}, {"d", 32}, {"f", 33}, {"g", 34}, {"h", 35},
        {"j", 36}, {"k", 37}, {"l", 38}, {";", 39}, {"'", 40},
        {"z", 44}, {"x", 45}, {"c", 46}, {"v", 47}, {"b", 48}, {"n", 49},
        {"m", 50}, {",", 51}, {".", 52}, {"/", 53}, {"\\", 43},
    };
    auto it = t.constFind(name);
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
    m_engine = std::make_unique<SuggestionEngine>();

    // Reflect whatever IM fcitx5 is already active on, rather than assuming
    // English.
    m_layout = m_fcitx->currentIM();
    m_layoutLabel = m_fcitx->shortLabel(m_layout);
    m_engine->setInputMethod(m_layout);
    regenerateLayout();
    emit layoutChanged();

    // skvirt owns its own visibility policy (touch → show, mouse/lost focus →
    // hide) and injects keys as a plain virtual keyboard. fcitx is never told
    // anything, so it stays in its normal mode (tray + candidate window).
    connect(m_input.get(), &InputMonitor::touchActivity,
            this, &KeyboardController::onTouchActivity);
    connect(m_input.get(), &InputMonitor::pointerActivity,
            this, &KeyboardController::onPointerActivity);
    connect(m_input.get(), &InputMonitor::tabletModeChanged,
            this, &KeyboardController::onTabletModeChanged);
    connect(m_kwin.get(), &KWinVk::textInputFocusChanged,
            this, &KeyboardController::onFieldFocusChanged);

    m_fieldFocused = m_kwin->textInputFocused();
    m_tabletMode = m_input->tabletMode();
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
    if (SkvirtSettings::showOnlyInTabletMode() && !m_tabletMode)
        return;  // setting gates auto-show to tablet (convertible) mode
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
    // The user reached for the mouse/touchpad. We always leave touch mode —
    // the next finger tap must re-trigger the show path — but the panel only
    // gets out of the way when the user asked for that (hideOnMouseMove).
    m_touchMode = false;
    if (SkvirtSettings::hideOnMouseMove())
        hideKeyboard();
}

void KeyboardController::onTabletModeChanged(bool tabletMode)
{
    m_tabletMode = tabletMode;
    if (!SkvirtSettings::showOnlyInTabletMode())
        return;  // tablet mode only matters to the engine when gated on it
    if (!tabletMode)
        hideKeyboard();                  // folded back to laptop: get out of the way
    else if (m_touchMode && m_fieldFocused)
        showKeyboard();                  // flipped to tablet with a field ready: pop up
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
    if (physKeyForChar(ch, pk)) {
        // Inject the US-position key; the active fcitx layout/engine decides
        // the actual output (latin, cyrillic, pinyin composition, …).
        m_kbd->tap(pk.keycode, pk.shift, heldModifiers());
    } else {
        // No US-layout key produces this character (accents, CJK, …): route
        // it through fcitx5's unicode addon instead of dropping it.
        m_kbd->typeUnicode(text);
    }

    updateWordBuffer(ch);

    if (m_shift) {  // one-shot shift
        m_shift = false;
        emit shiftActiveChanged();
    }
    clearModifiers();
}

void KeyboardController::commitKeyAt(const QString &keyName, bool shifted, const QString &producedChar)
{
    if (!m_kbd)
        return;

    int keycode;
    if (evdevForKeyName(keyName, keycode)) {
        // Inject the physical key at this position; the active fcitx layout
        // decides the actual character, so non-US letters (ü, é, й, …) work
        // without a char→key mapping.
        m_kbd->tap(keycode, shifted, heldModifiers());
    } else {
        // Unknown positional name: fall back to the character path.
        commitText(producedChar);
        return;  // commitText already handled buffer + one-shot shift
    }

    if (!producedChar.isEmpty()) {
        const QChar c = producedChar.at(0);
        updateWordBuffer(c.isLetter() ? c.toLower() : c);
    }

    if (m_shift) {  // one-shot shift
        m_shift = false;
        emit shiftActiveChanged();
    }
    clearModifiers();
}

void KeyboardController::regenerateLayout()
{
    m_generatedRows.clear();
    // Non-keyboard IMs (pinyin, …) compose from latin letters on a QWERTY
    // panel; an empty generatedRows keeps QML on its hardcoded fallback.
    if (!m_fcitx || !m_layout.startsWith(QLatin1String("keyboard-")))
        return;
    const QString xkbLayout = m_fcitx->layoutForIM(m_layout);
    if (!xkbLayout.isEmpty())
        m_generatedRows = m_layoutGen.generate(xkbLayout);
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
    m_kbd->tap(keycode, m_shift, heldModifiers());
    if (m_shift) {  // one-shot shift (⇧⇥, ⌘⇧↩, …)
        m_shift = false;
        emit shiftActiveChanged();
    }
    clearModifiers();

    // Keep the current-word buffer in sync with what the app received.
    const QString key = name.toLower();
    if (key == QLatin1String("backspace")) {
        if (!m_word.isEmpty()) {
            m_word.chop(1);
            refreshSuggestions();
        }
    } else if (key == QLatin1String("space") || key == QLatin1String("enter")
               || key == QLatin1String("tab") || key == QLatin1String("escape")) {
        if (!m_word.isEmpty()) {
            m_word.clear();
            refreshSuggestions();
        }
    }
}

// Track the word being typed so the SuggestionEngine has a prefix to complete.
// Pinyin composes from plain latin letters; hunspell layouts from letters of
// the alphabet at hand — anything else terminates the word.
void KeyboardController::updateWordBuffer(QChar committed)
{
    const bool wordChar = m_engine->isPinyinBackend()
        ? (committed >= QLatin1Char('a') && committed <= QLatin1Char('z'))
          || (committed >= QLatin1Char('A') && committed <= QLatin1Char('Z'))
        : committed.isLetter();
    if (wordChar) {
        if (m_word.size() < 32)   // bound the per-keystroke lookup work
            m_word.append(committed);
    } else if (!m_word.isEmpty()) {
        m_word.clear();
    }
    refreshSuggestions();
}

void KeyboardController::refreshSuggestions()
{
    const QStringList next = m_word.isEmpty()
        ? QStringList{}
        : m_engine->suggest(m_word, m_engine->isPinyinBackend() ? 7 : 5);
    if (next == m_suggestions)
        return;
    m_suggestions = next;
    emit suggestionsChanged();
}

void KeyboardController::commitSuggestion(int index)
{
    if (!m_kbd || !m_engine || index < 0 || index >= m_suggestions.size())
        return;
    const QString candidate = m_suggestions.at(index);

    if (m_engine->isPinyinBackend()) {
        // Escape cancels fcitx's live preedit built from the latin buffer we
        // injected, then the candidate is committed through the unicode addon.
        // Tradeoff: fcitx never sees this selection, so its frequency/history
        // learning is not updated by bar taps — accepted for now.
        m_kbd->tap(1, false);  // escape
        m_kbd->typeUnicode(candidate);
        m_word.clear();
    } else {
        // Type only the missing tail (common prefix with the buffer is already
        // in the app), then a space to end the word.
        int common = 0;
        while (common < candidate.size() && common < m_word.size()
               && candidate.at(common).toLower() == m_word.at(common).toLower())
            ++common;
        const QString tail = candidate.mid(common);
        for (const QChar ch : tail)
            commitText(QString(ch));  // reuses the physKey / unicode path
        m_kbd->tap(57, false);        // space
        m_word.clear();
    }
    refreshSuggestions();
}

void KeyboardController::toggleShift()
{
    m_shift = !m_shift;
    emit shiftActiveChanged();
}

// Latched modifiers work like the one-shot shift: a tap arms them, the next
// key is injected with them held, then they disarm. Evdev codes are spelled
// out to keep this file free of <linux/input.h>, as elsewhere here.
QList<int> KeyboardController::heldModifiers() const
{
    QList<int> mods;
    if (m_control)
        mods << 29;   // KEY_LEFTCTRL
    if (m_option)
        mods << 56;   // KEY_LEFTALT
    if (m_command)
        mods << 125;  // KEY_LEFTMETA
    return mods;
}

void KeyboardController::clearModifiers()
{
    if (!m_control && !m_option && !m_command)
        return;
    m_control = m_option = m_command = false;
    emit modifiersChanged();
}

void KeyboardController::toggleModifier(const QString &name)
{
    if (name == QLatin1String("control"))
        m_control = !m_control;
    else if (name == QLatin1String("option"))
        m_option = !m_option;
    else if (name == QLatin1String("command"))
        m_command = !m_command;
    else
        return;
    emit modifiersChanged();
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
    m_engine->setInputMethod(next);
    m_word.clear();
    refreshSuggestions();
    regenerateLayout();
    emit layoutChanged();
}

void KeyboardController::hidePanel()
{
    // Manual dismiss (▼): drop touch mode so it doesn't immediately reappear
    // while the same field keeps focus — the user asks again by tapping it.
    m_touchMode = false;
    hideKeyboard();
}
