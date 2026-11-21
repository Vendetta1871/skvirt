#include "KWinVk.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusVariant>
#include <QDebug>

static const char *kService = "org.kde.KWin";
static const char *kPath    = "/VirtualKeyboard";
static const char *kIface   = "org.kde.kwin.VirtualKeyboard";

KWinVk::KWinVk(QObject *parent) : QObject(parent)
{
    auto bus = QDBusConnection::sessionBus();

    // KWin emits an argument-less change signal on the interface; re-read the
    // property whenever activation state may have changed.
    bus.connect(kService, kPath, kIface, "activeChanged",
                this, SLOT(refresh()));

    refresh();
}

void KWinVk::refresh()
{
    QDBusInterface iface(kService, kPath, kIface,
                         QDBusConnection::sessionBus());
    // `active` flips when the focused app enables/disables text input on an
    // actual editable (text-input-v3 enable), i.e. the caret is in a field.
    // Not `activeClientSupportsTextInput` — that only says the client *bound*
    // the text-input protocol, which is true for nearly every Qt/GTK window
    // all the time, so gating on it pops the keyboard on any tap anywhere.
    // Our window never takes focus (WindowDoesNotAcceptFocus + layer-shell
    // KeyboardInteractivityNone), so mapping the keyboard does not flap it.
    // Known limits: terminals keep text input enabled while focused (they are
    // one big field), and XWayland/no-text-input clients never set it.
    const bool focused = iface.property("active").toBool();
    if (focused == m_focused)
        return;
    m_focused = focused;
    qDebug() << "skvirt: text-input focus" << (focused ? "gained" : "lost");
    emit textInputFocusChanged(m_focused);
}

// Both lookups run only when the panel is about to be shown (or once per
// device), so a short blocking call is fine; the timeout keeps a hung KWin
// from freezing the panel.
static constexpr int kCallTimeoutMs = 300;

QString KWinVk::deviceOutputName(const QString &sysName) const
{
    auto msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService),
        QStringLiteral("/org/kde/KWin/InputDevice/") + sysName,
        QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    msg << QStringLiteral("org.kde.KWin.InputDevice") << QStringLiteral("outputName");
    const QDBusReply<QDBusVariant> reply =
        QDBusConnection::sessionBus().call(msg, QDBus::Block, kCallTimeoutMs);
    return reply.isValid() ? reply.value().variant().toString() : QString();
}

QString KWinVk::activeOutputName() const
{
    const auto msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService), QStringLiteral("/KWin"),
        QStringLiteral("org.kde.KWin"), QStringLiteral("activeOutputName"));
    const QDBusReply<QString> reply =
        QDBusConnection::sessionBus().call(msg, QDBus::Block, kCallTimeoutMs);
    return reply.isValid() ? reply.value() : QString();
}
