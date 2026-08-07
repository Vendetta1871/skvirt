#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>

class Hunspell;

namespace libime {
class PinyinIME;
}

// Autosuggestion backend for the on-screen keyboard. Two engines, picked by
// the active fcitx input method:
//  - keyboard-* layouts: prefix completion over the Hunspell .dic word list
//    (keyboard-ru -> ru_RU, everything else -> en_US), with Hunspell::suggest()
//    as a typo fallback. Missing dictionaries simply yield no suggestions.
//  - anything else (pinyin/shuangpin/...): libime PinyinIME with the system
//    sc.dict + zh_CN.lm — the same engine and data fcitx5-chinese-addons uses,
//    so candidates match fcitx's own list (minus personal history).
//
// Synchronous by design: word lists are small-ish and lookups run per
// keystroke; loaded structures are cached across calls.
class SuggestionEngine
{
public:
    SuggestionEngine();
    ~SuggestionEngine();

    // Select the backend from the fcitx IM unique name (e.g. "keyboard-us",
    // "keyboard-ru", "pinyin"). Dictionaries reload lazily on next suggest().
    void setInputMethod(const QString &imName);

    bool isPinyinBackend() const { return m_pinyin; }

    // Up to maxResults completions for the current word/pinyin buffer.
    QStringList suggest(const QString &prefix, int maxResults);

private:
    void loadHunspellDict();   // lazy (re)load of m_words for m_dictName
    void ensureHunspell();     // lazy Hunspell object for the typo fallback
    bool ensurePinyin();       // lazy PinyinIME construction; false if data missing

    bool m_pinyin = false;
    QString m_dictName = QStringLiteral("en_US"); // hunspell dictionary base name
    bool m_dictDirty = true;

    QVector<QString> m_words;  // sorted (case-insensitive) word list from .dic
    QString m_affPath;         // kept for the lazy Hunspell fallback object
    QString m_dicPath;
    std::unique_ptr<Hunspell> m_hunspell;

    std::unique_ptr<libime::PinyinIME> m_pinyinIme;
    bool m_pinyinFailed = false; // data files missing / load threw: stay silent
};
