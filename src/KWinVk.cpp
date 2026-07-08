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
