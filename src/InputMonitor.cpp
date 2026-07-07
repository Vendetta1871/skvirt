#include "InputMonitor.h"

#include <QSocketNotifier>
#include <QDir>
#include <QDebug>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <cstring>

namespace {

constexpr int kBitsPerLong = 8 * sizeof(long);

inline bool testBit(const unsigned long *arr, int bit)
{
    return (arr[bit / kBitsPerLong] >> (bit % kBitsPerLong)) & 1ul;
}

template <int Max>
struct BitArray { unsigned long v[(Max + kBitsPerLong - 1) / kBitsPerLong] = {0}; };

} // namespace

InputMonitor::InputMonitor(QObject *parent) : QObject(parent)
{
    openDevices();
}

InputMonitor::~InputMonitor()
{
    for (const Dev &d : m_devs) {
        if (d.notifier)
            d.notifier->setEnabled(false);
        if (d.fd >= 0)
            ::close(d.fd);
    }
}

void InputMonitor::openDevices()
{
    const QDir dir("/dev/input");
    const QStringList nodes = dir.entryList({"event*"}, QDir::System, QDir::Name);

    for (const QString &name : nodes) {
        const QByteArray path = dir.absoluteFilePath(name).toLocal8Bit();
        int fd = ::open(path.constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
            continue;  // permission denied / busy — skip quietly

        BitArray<INPUT_PROP_CNT> props;
        BitArray<EV_CNT> evs;
        BitArray<ABS_CNT> abs;
        ioctl(fd, EVIOCGPROP(sizeof props.v), props.v);
        ioctl(fd, EVIOCGBIT(0, sizeof evs.v), evs.v);

        const bool direct  = testBit(props.v, INPUT_PROP_DIRECT);
        const bool pointer = testBit(props.v, INPUT_PROP_POINTER);
        const bool hasRel  = testBit(evs.v, EV_REL);
        bool hasAbsX = false, hasAbsMT = false;
        if (testBit(evs.v, EV_ABS)) {
            ioctl(fd, EVIOCGBIT(EV_ABS, sizeof abs.v), abs.v);
            hasAbsX  = testBit(abs.v, ABS_X);
            hasAbsMT = testBit(abs.v, ABS_MT_POSITION_X);
        }

        Kind kind = Ignore;
        if (direct && (hasAbsMT || hasAbsX))
            kind = Touch;            // touchscreen / pen digitizer
        else if (pointer || hasRel)
            kind = Pointer;          // mouse, touchpad, trackpoint

        if (kind == Ignore) {
            ::close(fd);
            continue;
        }

        auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this,
                [this, fd, kind]() { onReadable(fd, kind); });
        m_devs.push_back({fd, kind, notifier});
        if (kind == Touch)
            m_hasTouch = true;

        char nameBuf[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof nameBuf), nameBuf);
        qDebug() << "skvirt: monitoring" << path
                 << (kind == Touch ? "[touch]" : "[pointer]") << nameBuf;
    }

    if (!m_hasTouch)
        qWarning() << "skvirt: no readable touchscreen found — touch auto-show "
                      "disabled (is the user in the 'input' group?)";
}

void InputMonitor::onReadable(int fd, Kind kind)
{
    input_event evs[64];
    bool touch = false, pointer = false;

    for (;;) {
        ssize_t n = ::read(fd, evs, sizeof evs);
        if (n <= 0)
            break;
        const int count = n / sizeof(input_event);
        for (int i = 0; i < count; ++i) {
            const input_event &e = evs[i];
            if (kind == Touch) {
                // A new finger/pen contact: BTN_TOUCH press or a fresh MT slot.
                if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 1)
                    touch = true;
                else if (e.type == EV_ABS && e.code == ABS_MT_TRACKING_ID &&
                         e.value >= 0)
                    touch = true;
            } else {  // Pointer
                if (e.type == EV_REL)
                    pointer = true;
                else if (e.type == EV_KEY && e.value == 1 &&
                         (e.code == BTN_LEFT || e.code == BTN_RIGHT ||
                          e.code == BTN_MIDDLE || e.code == BTN_TOUCH))
                    pointer = true;
            }
        }
        if (n < static_cast<ssize_t>(sizeof evs))
            break;  // drained
    }

    if (touch)
        emit touchActivity();
    if (pointer)
        emit pointerActivity();
}
