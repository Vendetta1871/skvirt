#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>

// Talks to fcitx5's Controller1 D-Bus interface to read and switch the
// active input method. skvirt reads the group's IM list on demand so the
// language key always reflects however fcitx5 is actually configured
// (whatever IMs the user has enabled in their current group), and always
// explicitly SetCurrentIM's rather than relying on fcitx5's own Toggle, so
// skvirt stays the single source of truth for which IM is active.
class FcitxIm : public QObject
{
    Q_OBJECT
public:
    explicit FcitxIm(QObject *parent = nullptr);

    // Ordered unique IM names configured in the current group, e.g.
    // ["keyboard-us", "keyboard-ru", "pinyin"].
    QStringList groupInputMethods() const;

    // Currently active unique IM name (e.g. "keyboard-ru").
    QString currentIM() const;

    // Short display code for a unique IM name (e.g. "en", "ru", "拼"), taken
    // from fcitx5's own AvailableInputMethods() label field. Lazily fetches
    // and caches the full system IM list on first call.
    QString shortLabel(const QString &uniqueName);

    // xkb layout backing a keyboard-* IM: the group's layout override when
    // fcitx5 sets one, else the suffix of the "keyboard-XX" unique name.
    // {} for non-keyboard IMs (pinyin, …).
    QString layoutForIM(const QString &uniqueName) const;

    // Fire-and-forget switch; does not wait for fcitx5 to finish applying it.
    void setCurrentIM(const QString &uniqueName);

private:
    void ensureLabelCache();

    QHash<QString, QString> m_labelCache;
    bool m_labelCacheLoaded = false;
};
