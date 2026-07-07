#include "KWinVk.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QDebug>

static const char *kService = "org.kde.KWin";
static const char *kPath    = "/VirtualKeyboard";
static const char *kIface   = "org.kde.kwin.VirtualKeyboard";

KWinVk::KWinVk(QObject *parent) : QObject(parent)
{
    auto bus = QDBusConnection::sessionBus();

    // KWin emits argument-less change signals on the interface; re-read the
    // property whenever focus/activation state may have changed.
    for (const char *sig : {"activeClientSupportsTextInputChanged",
                            "activeChanged"}) {
        bus.connect(kService, kPath, kIface, sig,
                    this, SLOT(refresh()));
    }

    refresh();
}

void KWinVk::refresh()
{
    QDBusInterface iface(kService, kPath, kIface,
                         QDBusConnection::sessionBus());
    // Whether the focused window accepts text input. We deliberately do NOT
    // gate on the `active` property: KWin flips it to false the moment our own
    // layer-shell keyboard maps, which would hide us immediately. This one
    // tracks the app being typed into and is stable across our show/hide.
    const bool focused =
        iface.property("activeClientSupportsTextInput").toBool();
    if (focused == m_focused)
        return;
    m_focused = focused;
    qDebug() << "skvirt: text-input focus" << (focused ? "gained" : "lost");
    emit textInputFocusChanged(m_focused);
}
