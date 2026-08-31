#include "LayoutGenerator.h"

#include <QFile>
#include <QHash>
#include <QList>
#include <QMap>
#include <QRegularExpression>

namespace {

// One generated key: its xkb physical code, the US-position name the
// controller commits by, and its width factor in key units.
struct KeyPos { const char *code; const char *name; double width; };

// The four typing rows of an ANSI/Mac keyboard, in panel order. The function
// keys around them (tab, caps lock, shift, return, delete, …) are fixed and
// live in QML; everything here follows the active xkb layout.
const QList<KeyPos> kRows[4] = {
    // Number row: TLDE + AE01..AE12.
    {{"TLDE", "`", 1.0},
     {"AE01", "1", 1.0}, {"AE02", "2", 1.0}, {"AE03", "3", 1.0},
     {"AE04", "4", 1.0}, {"AE05", "5", 1.0}, {"AE06", "6", 1.0},
     {"AE07", "7", 1.0}, {"AE08", "8", 1.0}, {"AE09", "9", 1.0},
     {"AE10", "0", 1.0}, {"AE11", "-", 1.0}, {"AE12", "=", 1.0}},
    // Tab row: AD01..AD12 + the backslash key that closes it on ANSI.
    {{"AD01", "q", 1.0}, {"AD02", "w", 1.0}, {"AD03", "e", 1.0},
     {"AD04", "r", 1.0}, {"AD05", "t", 1.0}, {"AD06", "y", 1.0},
     {"AD07", "u", 1.0}, {"AD08", "i", 1.0}, {"AD09", "o", 1.0},
     {"AD10", "p", 1.0}, {"AD11", "[", 1.0}, {"AD12", "]", 1.0},
     {"BKSL", "\\", 1.5}},
    // Home row: AC01..AC11.
    {{"AC01", "a", 1.0}, {"AC02", "s", 1.0}, {"AC03", "d", 1.0},
     {"AC04", "f", 1.0}, {"AC05", "g", 1.0}, {"AC06", "h", 1.0},
     {"AC07", "j", 1.0}, {"AC08", "k", 1.0}, {"AC09", "l", 1.0},
     {"AC10", ";", 1.0}, {"AC11", "'", 1.0}},
    // Bottom letter row: AB01..AB10.
    {{"AB01", "z", 1.0}, {"AB02", "x", 1.0}, {"AB03", "c", 1.0},
     {"AB04", "v", 1.0}, {"AB05", "b", 1.0}, {"AB06", "n", 1.0},
     {"AB07", "m", 1.0}, {"AB08", ",", 1.0}, {"AB09", ".", 1.0},
     {"AB10", "/", 1.0}},
};

// ---- keysym name -> character ---------------------------------------------

// One XK_ entry: the keysym value plus the Unicode codepoint from the
// trailing /* U+XXXX */ comment (0 when the header doesn't annotate one).
struct KeysymInfo { uint value; uint ucs; };

// XK_ name -> KeysymInfo, parsed lazily once from the system keysymdef.h.
// The U+ annotations make every legacy range (Cyrillic, Greek, Arabic, Thai,
// Hebrew, …) resolvable without per-range lookup tables. If the file is
// missing, a minimal builtin punctuation table is used (letters and digits
// resolve via the single-char literal rule anyway).
const QHash<QString, KeysymInfo> &keysymTable()
{
    static QHash<QString, KeysymInfo> table;
    static bool loaded = false;
    if (loaded)
        return table;
    loaded = true;

    QFile f(QStringLiteral("/usr/include/X11/keysymdef.h"));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        static const QRegularExpression re(
            QStringLiteral("^#define\\s+XK_(\\w+)\\s+0x([0-9a-fA-F]+)"
                           "(?:\\s+/\\*\\s*U\\+([0-9a-fA-F]{4,6}))?"));
        while (!f.atEnd()) {
            const auto m = re.match(QString::fromUtf8(f.readLine()));
            if (m.hasMatch())
                table.insert(m.captured(1),
                             {m.captured(2).toUInt(nullptr, 16),
                              m.captured(3).toUInt(nullptr, 16)});
        }
    } else {
        const struct { const char *name; uint value; } builtin[] = {
            {"exclam", 0x21}, {"quotedbl", 0x22}, {"numbersign", 0x23},
            {"dollar", 0x24}, {"percent", 0x25}, {"ampersand", 0x26},
            {"apostrophe", 0x27}, {"parenleft", 0x28}, {"parenright", 0x29},
            {"asterisk", 0x2A}, {"plus", 0x2B}, {"comma", 0x2C},
            {"minus", 0x2D}, {"period", 0x2E}, {"slash", 0x2F},
            {"colon", 0x3A}, {"semicolon", 0x3B}, {"less", 0x3C},
            {"equal", 0x3D}, {"greater", 0x3E}, {"question", 0x3F},
            {"at", 0x40}, {"bracketleft", 0x5B}, {"backslash", 0x5C},
            {"bracketright", 0x5D}, {"asciicircum", 0x5E}, {"underscore", 0x5F},
            {"grave", 0x60}, {"braceleft", 0x7B}, {"bar", 0x7C},
            {"braceright", 0x7D}, {"asciitilde", 0x7E},
        };
        for (const auto &e : builtin)
            table.insert(QLatin1String(e.name), {e.value, 0});
    }
    return table;
}

QChar codepointToChar(uint cp)
{
    if (cp < 0x20 || cp == 0x7F || (cp >= 0x80 && cp < 0xA0)
        || cp > 0x10FFFF || QChar::isSurrogate(cp))
        return QChar();
    return QChar(cp);
}

// Resolve one keysym name ("Cyrillic_ie", "udiaeresis", "e", "U20BD", …) to
// the character it renders. Returns a null QChar for anything unprintable
// (NoSymbol/VoidSymbol, dead keys, modifiers, unmapped values).
QChar keysymToChar(const QString &sym)
{
    if (sym.isEmpty())
        return QChar();

    // Dead keys are real keys on the physical keyboard, and injecting their
    // keycode composes correctly through xkb — they just have no character of
    // their own, so they are drawn with the spacing form of their diacritic
    // (^ ´ ¨ …) the way a German or French keyboard prints them. Dead keys
    // outside this table are dropped rather than mislabelled.
    if (sym.startsWith(QLatin1String("dead_"))) {
        static const QHash<QString, ushort> deadChars = {
            {QStringLiteral("dead_grave"), 0x60},       {QStringLiteral("dead_acute"), 0xB4},
            {QStringLiteral("dead_circumflex"), 0x5E},  {QStringLiteral("dead_tilde"), 0x7E},
            {QStringLiteral("dead_macron"), 0xAF},      {QStringLiteral("dead_breve"), 0x2D8},
            {QStringLiteral("dead_abovedot"), 0x2D9},   {QStringLiteral("dead_diaeresis"), 0xA8},
            {QStringLiteral("dead_abovering"), 0xB0},   {QStringLiteral("dead_doubleacute"), 0x2DD},
            {QStringLiteral("dead_caron"), 0x2C7},      {QStringLiteral("dead_cedilla"), 0xB8},
            {QStringLiteral("dead_ogonek"), 0x2DB},     {QStringLiteral("dead_currency"), 0xA4},
        };
        const auto it = deadChars.constFind(sym);
        return it == deadChars.constEnd() ? QChar() : QChar(*it);
    }

    // Single-character names are the character itself ("e", "3", …).
    if (sym.size() == 1)
        return sym.at(0);

    // U-keysyms carry a Unicode codepoint directly ("U20BD").
    if (sym.at(0) == QLatin1Char('U') && sym.size() >= 5 && sym.size() <= 7) {
        bool ok = false;
        const uint cp = sym.mid(1).toUInt(&ok, 16);
        if (ok) {
            if (const QChar c = codepointToChar(cp); !c.isNull())
                return c;
        }
        // Not actually a U-keysym (e.g. "Udiaeresis"): fall through.
    }

    const auto it = keysymTable().constFind(sym);
    if (it == keysymTable().constEnd())
        return QChar();
    // The header's own U+ annotation covers every legacy range.
    if (it->ucs) {
        if (const QChar c = codepointToChar(it->ucs); !c.isNull())
            return c;
    }
    const uint v = it->value;
    if (v >= 0x01000000 && v <= 0x0110FFFF)
        return codepointToChar(v - 0x01000000);  // X11 Unicode keysym convention
    if ((v >= 0x20 && v <= 0x7E) || (v >= 0xA0 && v <= 0xFF))
        return QChar(v);                          // printable Latin-1
    return QChar();                               // control chars, VoidSymbol, …
}

// ---- xkb_symbols block parsing ---------------------------------------------

QString readSymbolsFile(const QString &name, QHash<QString, QString> *cache)
{
    auto it = cache->find(name);
    if (it != cache->end())
        return it.value();
    QFile f(QStringLiteral("/usr/share/X11/xkb/symbols/") + name);
    QString text;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        text = QString::fromUtf8(f.readAll());
        static const QRegularExpression commentRe(QStringLiteral("//[^\n]*"));
        text.remove(commentRe);
    }
    cache->insert(name, text);
    return text;
}

// Body (between the outer braces) of the xkb_symbols block named `section`,
// or of the first block when `section` is empty. {} when not found.
QString blockBody(const QString &text, const QString &section)
{
    static const QRegularExpression re(QStringLiteral("xkb_symbols\\s+\"([^\"]+)\""));
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        if (!section.isEmpty() && m.captured(1) != section)
            continue;
        const int open = text.indexOf(QLatin1Char('{'), m.capturedEnd());
        if (open < 0)
            return {};
        int depth = 1;
        int i = open + 1;
        for (; i < text.size() && depth > 0; ++i) {
            if (text.at(i) == QLatin1Char('{'))
                ++depth;
            else if (text.at(i) == QLatin1Char('}'))
                --depth;
        }
        if (depth != 0)
            return {};
        return text.mid(open + 1, i - open - 2);
    }
    return {};
}

// Collect `key <CODE> { [ sym1, sym2, ... ] };` lines from one block into
// `acc` (phys code -> group-1 symbols). Include directives are resolved
// first, recursively, so the block's own keys override the included base —
// mirroring how xkb merges sections ("de(basic)" over "latin(type4)" etc.).
void collectKeys(const QString &file, const QString &section, int depth,
                 QMap<QString, QStringList> &acc, QHash<QString, QString> &cache)
{
    if (depth > 6)
        return;
    const QString text = readSymbolsFile(file, &cache);
    if (text.isEmpty())
        return;
    const QString body = blockBody(text, section);
    if (body.isEmpty())
        return;

    static const QRegularExpression incRe(QStringLiteral("include\\s+\"([^\"]+)\""));
    static const QRegularExpression incTokRe(QStringLiteral("^([^(]+)(?:\\(([^)]+)\\))?$"));
    auto ii = incRe.globalMatch(body);
    while (ii.hasNext()) {
        const QStringList tokens = ii.next().captured(1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &tok : tokens) {
            const auto tm = incTokRe.match(tok);
            if (tm.hasMatch())
                collectKeys(tm.captured(1), tm.captured(2), depth + 1, acc, cache);
        }
    }

    // Tolerate a `[Group1]` qualifier before the braces, whitespace/newlines
    // anywhere, and trailing type= levels: only the first [...] group counts.
    static const QRegularExpression keyRe(
        QStringLiteral("key\\s*<(\\w+)>\\s*(?:\\[[^\\]]*\\]\\s*)?\\{\\s*\\[([^\\]]*)\\]"),
        QRegularExpression::DotMatchesEverythingOption);
    auto ki = keyRe.globalMatch(body);
    while (ki.hasNext()) {
        const auto m = ki.next();
        QStringList syms;
        const QStringList parts = m.captured(2).split(QLatin1Char(','));
        for (const QString &p : parts) {
            const QString s = p.trimmed();
            if (!s.isEmpty())
                syms << s;
        }
        if (!syms.isEmpty())
            acc.insert(m.captured(1), syms);
    }
}

} // namespace

QVariantList LayoutGenerator::generate(const QString &layoutName)
{
    // fcitx layout overrides may carry a variant ("us(intl)"): only the base
    // layout selects the file. Reject anything that could escape the symbols
    // directory.
    const QString base = layoutName.section(QLatin1Char('('), 0, 0);
    if (base.isEmpty() || base.contains(QLatin1Char('/')) || base.contains(QLatin1String("..")))
        return {};

    QHash<QString, QString> cache;
    const QString text = readSymbolsFile(base, &cache);
    if (text.isEmpty())
        return {};

    // "basic" is the conventional default section; fall back to the first
    // block (e.g. ru's default section is "winkeys").
    const QString section = blockBody(text, QStringLiteral("basic")).isEmpty()
        ? QString()
        : QStringLiteral("basic");

    QMap<QString, QStringList> keys;
    collectKeys(base, section, 0, keys, cache);

    QVariantList rows;
    for (const QList<KeyPos> &positions : kRows) {
        QVariantList row;
        for (const KeyPos &pos : positions) {
            const auto it = keys.constFind(QLatin1String(pos.code));
            if (it == keys.constEnd())
                continue;
            const QStringList &syms = it.value();
            const QChar lower = keysymToChar(syms.value(0));
            if (lower.isNull())
                continue;  // unprintable key: drop it, keep the row clean
            const QChar upper = syms.size() > 1 ? keysymToChar(syms.at(1)) : QChar();
            const QString shiftLabel =
                (!upper.isNull() && upper != lower) ? QString(upper) : QString();
            // QVariant::fromValue: plain append() would flatten the list.
            row.append(QVariant::fromValue(QVariantList{
                QLatin1String(pos.name), QString(lower), shiftLabel, pos.width}));
        }
        rows.append(QVariant::fromValue(row));
    }

    // Sanity check: a real alphabetic layout has a near-full top letter row.
    if (rows.at(1).toList().size() < 7)
        return {};
    return rows;
}
