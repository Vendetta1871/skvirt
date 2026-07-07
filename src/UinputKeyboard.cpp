#include "UinputKeyboard.h"

#include <QDebug>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <cstring>

UinputKeyboard::UinputKeyboard(QObject *parent) : QObject(parent)
{
    m_fd = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (m_fd < 0) {
        qWarning() << "skvirt: cannot open /dev/uinput — key injection disabled "
                      "(is the user in the 'input' group?)";
        return;
    }

    ioctl(m_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(m_fd, UI_SET_EVBIT, EV_SYN);
    // Enable the full standard key range; we only ever emit a known subset.
    for (int code = 1; code <= 248; ++code)
        ioctl(m_fd, UI_SET_KEYBIT, code);

    uinput_setup usetup;
    std::memset(&usetup, 0, sizeof usetup);
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x5352;  // "SR"
    usetup.id.product = 0x0001;
    std::strcpy(usetup.name, "skvirt virtual keyboard");

    if (ioctl(m_fd, UI_DEV_SETUP, &usetup) < 0 ||
        ioctl(m_fd, UI_DEV_CREATE) < 0) {
        qWarning() << "skvirt: uinput device setup failed";
        ::close(m_fd);
        m_fd = -1;
        return;
    }
    qDebug() << "skvirt: uinput virtual keyboard created";
}

UinputKeyboard::~UinputKeyboard()
{
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        ::close(m_fd);
    }
}

void UinputKeyboard::emitEvent(int type, int code, int value)
{
    input_event ev;
    std::memset(&ev, 0, sizeof ev);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    if (::write(m_fd, &ev, sizeof ev) < 0)
        qWarning() << "skvirt: uinput write failed";
}

void UinputKeyboard::tap(int keycode, bool shift)
{
    if (m_fd < 0)
        return;
    if (shift) {
        emitEvent(EV_KEY, KEY_LEFTSHIFT, 1);
        emitEvent(EV_SYN, SYN_REPORT, 0);
    }
    emitEvent(EV_KEY, keycode, 1);
    emitEvent(EV_SYN, SYN_REPORT, 0);
    emitEvent(EV_KEY, keycode, 0);
    emitEvent(EV_SYN, SYN_REPORT, 0);
    if (shift) {
        emitEvent(EV_KEY, KEY_LEFTSHIFT, 0);
        emitEvent(EV_SYN, SYN_REPORT, 0);
    }
}
