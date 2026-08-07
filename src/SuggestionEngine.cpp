#include "SuggestionEngine.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

#include <algorithm>

#include <hunspell.hxx>

// Qt's keyword macros clash with libime/fcitx headers (datrie foreach(),
// ConnectableObject::emit()).
#ifdef foreach
#undef foreach
#endif
#ifdef emit
#undef emit
#endif
#ifdef signals
#undef signals
#endif
#ifdef slots
#undef slots
#endif

#include <libime/pinyin/pinyinime.h>
#include <libime/pinyin/pinyindictionary.h>
#include <libime/pinyin/pinyincontext.h>
#include <libime/core/userlanguagemodel.h>

namespace {
// System pinyin dictionary + language model shipped by the libime package
// (the same files fcitx5-chinese-addons loads).
const char kPinyinDict[] = "/usr/share/libime/sc.dict";
const char kPinyinLM[]   = "/usr/lib/libime/zh_CN.lm";

// Locate <base>.<ext> in the usual hunspell dictionary directories.
QString findDictFile(const QString &base, const QString &ext)
{
    static const char *dirs[] = {
        "/usr/share/hunspell",
        "/usr/share/myspell",
        "/usr/local/share/hunspell",
    };
    for (const char *dir : dirs) {
        const QString path = QString::fromLatin1(dir) + QLatin1Char('/') + base
                           + QLatin1Char('.') + ext;
        if (QFileInfo::exists(path))
            return path;
    }
    return {};
}

bool caseInsensitiveLess(const QString &a, const QString &b)
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}
} // namespace

SuggestionEngine::SuggestionEngine() = default;

SuggestionEngine::~SuggestionEngine() = default;

void SuggestionEngine::setInputMethod(const QString &imName)
{
    // Non-"keyboard-*" IMs (pinyin, shuangpin, …) take latin input and are
    // served by the libime pinyin backend; keyboard layouts use hunspell.
    const bool pinyin = !imName.startsWith(QLatin1String("keyboard-"));
    const QString dictName = (imName == QLatin1String("keyboard-ru"))
        ? QStringLiteral("ru_RU") : QStringLiteral("en_US");
    if (pinyin != m_pinyin || dictName != m_dictName) {
        m_pinyin = pinyin;
        m_dictName = dictName;
        m_dictDirty = true;
        m_hunspell.reset();
    }
}

void SuggestionEngine::loadHunspellDict()
{
    m_words.clear();
    m_affPath = findDictFile(m_dictName, QStringLiteral("aff"));
    m_dicPath = findDictFile(m_dictName, QStringLiteral("dic"));
    if (m_dicPath.isEmpty()) {
        qDebug() << "skvirt: no hunspell dictionary for" << m_dictName
                 << "— suggestions disabled";
        return;
    }

    QFile file(m_dicPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);
    in.readLine();  // first line: approximate word count
    while (!in.atEnd()) {
        QString word = in.readLine();
        const int slash = word.indexOf(QLatin1Char('/'));   // strip "/flags"
        if (slash >= 0)
            word.truncate(slash);
        const int tab = word.indexOf(QLatin1Char('\t'));    // strip morphology
        if (tab >= 0)
            word.truncate(tab);
        word = word.trimmed();
        if (!word.isEmpty())
            m_words.append(word);
    }
    std::sort(m_words.begin(), m_words.end(), caseInsensitiveLess);
    qDebug() << "skvirt: loaded" << m_words.size() << "words from" << m_dicPath;
}

void SuggestionEngine::ensureHunspell()
{
    if (m_hunspell || m_affPath.isEmpty() || m_dicPath.isEmpty())
        return;
    m_hunspell = std::make_unique<Hunspell>(
        m_affPath.toUtf8().constData(), m_dicPath.toUtf8().constData());
}

bool SuggestionEngine::ensurePinyin()
{
    if (m_pinyinIme)
        return true;
    if (m_pinyinFailed)
        return false;
    if (!QFileInfo::exists(QString::fromLatin1(kPinyinDict))
        || !QFileInfo::exists(QString::fromLatin1(kPinyinLM))) {
        qWarning() << "skvirt: libime dictionary/LM not found — pinyin suggestions disabled";
        m_pinyinFailed = true;
        return false;
    }
    try {
        auto dict = std::make_unique<libime::PinyinDictionary>();
        dict->load(libime::PinyinDictionary::SystemDict, kPinyinDict,
                   libime::PinyinDictFormat::Binary);
        auto lm = std::make_unique<libime::UserLanguageModel>(kPinyinLM);
        m_pinyinIme = std::make_unique<libime::PinyinIME>(std::move(dict),
                                                          std::move(lm));
    } catch (const std::exception &e) {
        qWarning() << "skvirt: failed to init libime pinyin:" << e.what();
        m_pinyinFailed = true;
        return false;
    }
    return true;
}

QStringList SuggestionEngine::suggest(const QString &prefix, int maxResults)
{
    QStringList out;
    if (prefix.isEmpty() || maxResults <= 0)
        return out;

    if (m_pinyin) {
        if (!ensurePinyin())
            return out;
        libime::PinyinContext ctx(m_pinyinIme.get());
        ctx.type(prefix.toLower().toStdString());
        const auto &cands = ctx.candidates();
        for (const auto &cand : cands) {
            if (out.size() >= maxResults)
                break;
            const QString word = QString::fromStdString(cand.toString());
            if (!word.isEmpty())
                out << word;
        }
        return out;
    }

    // Hunspell backend: case-insensitive prefix completion over the sorted
    // .dic word list.
    if (m_dictDirty) {
        m_dictDirty = false;
        loadHunspellDict();
    }
    auto it = std::lower_bound(m_words.begin(), m_words.end(), prefix,
                               caseInsensitiveLess);
    for (; it != m_words.end() && out.size() < maxResults; ++it) {
        if (!it->startsWith(prefix, Qt::CaseInsensitive))
            break;
        if (!out.contains(*it, Qt::CaseInsensitive))  // skip case-only dupes
            out << *it;
    }

    // Typo fallback: nothing starts with the buffer — ask hunspell proper.
    if (out.isEmpty() && prefix.size() >= 3) {
        ensureHunspell();
        if (m_hunspell) {
            const auto words = m_hunspell->suggest(prefix.toUtf8().toStdString());
            for (const auto &word : words) {
                if (out.size() >= maxResults)
                    break;
                out << QString::fromStdString(word);
            }
        }
    }
    return out;
}
