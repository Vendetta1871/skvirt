#include "UinputKeyboard.h"

#include <QDebug>
#include <QHash>
#include <QThread>
#include <QVector>

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

void UinputKeyboard::typeUnicode(const QString &text)
{
    if (m_fd < 0)
        return;

    // Keycodes for hex entry: digits on KEY_1..KEY_0, letters a..f on their
    // alphabetic keys. All already enabled (the device claims codes 1..248).
    static const QHash<QChar, int> hexKeys = []{
        QHash<QChar, int> t;
        const int digits[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // KEY_1..KEY_0
        for (int i = 0; i < 9; ++i)
            t[QChar('1' + i)] = digits[i];
        t[QChar('0')] = 11;                                  // KEY_0
        const int letters[] = {30, 48, 46, 32, 18, 33};        // KEY_A..KEY_F
        for (int i = 0; i < 6; ++i)
            t[QChar('a' + i)] = letters[i];
        return t;
    }();

    // UCS-4 so non-BMP characters are one codepoint, not a surrogate pair.
    const QVector<uint> codepoints = text.toUcs4();
    for (uint cp : codepoints) {
        const QString hex = QString::number(cp, 16);  // lowercase, as fcitx shows it
        // fcitx5 unicode addon, DirectUnicodeMode (Ctrl+Shift+U): hex-only
        // entry, Enter commits the codepoint. Do NOT use the Ctrl+Alt+Shift+U
        // trigger — that opens search-by-name mode, where typed hex is a
        // search query and Enter picks an arbitrary match.
        emitEvent(EV_KEY, KEY_LEFTCTRL, 1);
        emitEvent(EV_KEY, KEY_LEFTSHIFT, 1);
        emitEvent(EV_SYN, SYN_REPORT, 0);
        tap(KEY_U, false);
        emitEvent(EV_KEY, KEY_LEFTSHIFT, 0);
        emitEvent(EV_KEY, KEY_LEFTCTRL, 0);
        emitEvent(EV_SYN, SYN_REPORT, 0);
        QThread::msleep(30);  // let fcitx enter unicode-input mode
        for (const QChar d : hex) {
            tap(hexKeys.value(d), false);
            QThread::msleep(5);
        }
        tap(KEY_ENTER, false);
        QThread::msleep(10);
    }
}
