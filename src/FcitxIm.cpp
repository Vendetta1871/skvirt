#include "FcitxIm.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMetaType>
#include <QDebug>

static const char *kService = "org.fcitx.Fcitx5";
static const char *kPath    = "/controller";
static const char *kIface   = "org.fcitx.Fcitx.Controller1";

namespace {

// One entry of InputMethodGroupInfo's IM list: (uniqueName, layoutOverride).
struct GroupIMEntry {
    QString uniqueName;
    QString layoutName;
};

QDBusArgument &operator<<(QDBusArgument &arg, const GroupIMEntry &e)
{
    arg.beginStructure();
    arg << e.uniqueName << e.layoutName;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, GroupIMEntry &e)
{
    arg.beginStructure();
    arg >> e.uniqueName >> e.layoutName;
    arg.endStructure();
    return arg;
}

// One entry of AvailableInputMethods: (uniqueName, name, nativeName, icon,
// label, languageCode, configurable).
struct AvailableIMEntry {
    QString uniqueName;
    QString name;
    QString nativeName;
    QString icon;
    QString label;
    QString languageCode;
    bool configurable;
};

QDBusArgument &operator<<(QDBusArgument &arg, const AvailableIMEntry &e)
{
    arg.beginStructure();
    arg << e.uniqueName << e.name << e.nativeName << e.icon << e.label
        << e.languageCode << e.configurable;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, AvailableIMEntry &e)
{
    arg.beginStructure();
    arg >> e.uniqueName >> e.name >> e.nativeName >> e.icon >> e.label
        >> e.languageCode >> e.configurable;
    arg.endStructure();
    return arg;
}

} // namespace

Q_DECLARE_METATYPE(GroupIMEntry)
Q_DECLARE_METATYPE(AvailableIMEntry)

FcitxIm::FcitxIm(QObject *parent) : QObject(parent)
{
    qDBusRegisterMetaType<GroupIMEntry>();
    qDBusRegisterMetaType<QList<GroupIMEntry>>();
    qDBusRegisterMetaType<AvailableIMEntry>();
    qDBusRegisterMetaType<QList<AvailableIMEntry>>();
}

// IM entries of the current group, or {} when fcitx5 is unreachable.
static QList<GroupIMEntry> groupEntries()
{
    QDBusInterface iface(kService, kPath, kIface, QDBusConnection::sessionBus());

    const QDBusMessage groupReply = iface.call(QStringLiteral("CurrentInputMethodGroup"));
    if (groupReply.type() != QDBusMessage::ReplyMessage || groupReply.arguments().isEmpty()) {
        qWarning() << "skvirt: CurrentInputMethodGroup failed:" << groupReply.errorMessage();
        return {};
    }
    const QString group = groupReply.arguments().at(0).toString();

    const QDBusMessage infoReply = iface.call(QStringLiteral("InputMethodGroupInfo"), group);
    if (infoReply.type() != QDBusMessage::ReplyMessage || infoReply.arguments().size() < 2) {
        qWarning() << "skvirt: InputMethodGroupInfo failed:" << infoReply.errorMessage();
        return {};
    }

    return qdbus_cast<QList<GroupIMEntry>>(
        infoReply.arguments().at(1).value<QDBusArgument>());
}

QStringList FcitxIm::groupInputMethods() const
{
    const auto entries = groupEntries();
    QStringList ims;
    for (const GroupIMEntry &e : entries)
        ims << e.uniqueName;
    return ims;
}

QString FcitxIm::layoutForIM(const QString &uniqueName) const
{
    const auto entries = groupEntries();
    for (const GroupIMEntry &e : entries) {
        if (e.uniqueName == uniqueName && !e.layoutName.isEmpty())
            return e.layoutName;
    }
    // keyboard-XX IMs default to the XX xkb layout when no override is set.
    static const QString kPrefix = QStringLiteral("keyboard-");
    if (uniqueName.startsWith(kPrefix))
        return uniqueName.mid(kPrefix.size());
    return {};
}

QString FcitxIm::currentIM() const
{
    QDBusInterface iface(kService, kPath, kIface, QDBusConnection::sessionBus());
    const QDBusMessage reply = iface.call(QStringLiteral("CurrentInputMethod"));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        qWarning() << "skvirt: CurrentInputMethod failed:" << reply.errorMessage();
        return {};
    }
    return reply.arguments().at(0).toString();
}

void FcitxIm::ensureLabelCache()
{
    if (m_labelCacheLoaded)
        return;
    m_labelCacheLoaded = true;

    QDBusInterface iface(kService, kPath, kIface, QDBusConnection::sessionBus());
    const QDBusMessage reply = iface.call(QStringLiteral("AvailableInputMethods"));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        qWarning() << "skvirt: AvailableInputMethods failed:" << reply.errorMessage();
        return;
    }

    const auto entries = qdbus_cast<QList<AvailableIMEntry>>(
        reply.arguments().at(0).value<QDBusArgument>());
    for (const AvailableIMEntry &e : entries)
        m_labelCache[e.uniqueName] = e.label;
}

QString FcitxIm::shortLabel(const QString &uniqueName)
{
    ensureLabelCache();
    return m_labelCache.value(uniqueName, uniqueName);
}

void FcitxIm::setCurrentIM(const QString &uniqueName)
{
    QDBusInterface iface(kService, kPath, kIface, QDBusConnection::sessionBus());
    iface.asyncCall(QStringLiteral("SetCurrentIM"), uniqueName);
}
