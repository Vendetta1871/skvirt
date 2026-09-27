#include "VoiceInput.h"

#include "skvirt.h"  // generated SkvirtSettings (kconfig_add_kcfg_files)

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QDataStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace {
// One take is capped so a stuck "hold" cannot record (and upload) forever.
constexpr int kMaxRecordMs = 120 * 1000;
// Capture keeps running this long after release: people let go of the key
// while still finishing the last syllable.
constexpr int kTailMs = 200;
// Clips shorter than this are silence/accidental releases: skip the upload.
constexpr qreal kMinClipSeconds = 0.3;
constexpr int kHealthTimeoutMs = 2000;
// Inference on a CPU-only server is slow (tens of seconds for a long take).
constexpr int kInferenceTimeoutMs = 120 * 1000;
}

VoiceInput::VoiceInput(QObject *parent) : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    m_limitTimer = new QTimer(this);
    m_limitTimer->setSingleShot(true);
    m_limitTimer->setInterval(kMaxRecordMs);
    connect(m_limitTimer, &QTimer::timeout, this, &VoiceInput::stopRecording);
    m_tailTimer = new QTimer(this);
    m_tailTimer->setSingleShot(true);
    m_tailTimer->setInterval(kTailMs);
    connect(m_tailTimer, &QTimer::timeout, this, &VoiceInput::finishCapture);
}

VoiceInput::~VoiceInput()
{
    cancelRecording();
}

// whisper-server endpoint: the configured address ("http://host:port", maybe
// with a path prefix behind a proxy; trailing slash trimmed) plus `path`.
// Appended rather than QUrl::resolved(), which would drop such a prefix.
static QUrl endpoint(const QString &path)
{
    QString url = SkvirtSettings::self()->whisperServerUrl().trimmed();
    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);
    return QUrl(url + path);
}

void VoiceInput::checkAvailability()
{
    if (m_probe)  // a probe is already in flight
        return;

    // Fresh from the config file so edits from System Settings apply
    // without a restart.
    SkvirtSettings::self()->load();
    QNetworkRequest req(endpoint(QStringLiteral("/health")));
    req.setTransferTimeout(kHealthTimeoutMs);
    m_probe = m_nam->get(req);
    connect(m_probe, &QNetworkReply::finished, this, [this] {
        m_probe->deleteLater();
        const bool ok = m_probe->error() == QNetworkReply::NoError
            && m_probe->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
        m_probe = nullptr;
        emit availabilityChanged(ok);
    });
}

bool VoiceInput::startRecording()
{
    if (m_recording)
        return true;

    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        qWarning() << "skvirt: voice input: no audio input device";
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(16000);   // whisper's native rate: smallest upload
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!device.isFormatSupported(format))
        format = device.preferredFormat();  // whisper resamples server-side

    m_source = new QAudioSource(device, format, this);
    m_pcm.clear();
    m_pcm.reserve(format.bytesForDuration(kMaxRecordMs * 1000) / 4);  // ~30 s

    m_captureIo = m_source->start();  // pull mode: we read() as data arrives
    if (!m_captureIo) {
        qWarning() << "skvirt: voice input: failed to start audio capture";
        delete m_source;
        m_source = nullptr;
        return false;
    }
    connect(m_captureIo, &QIODevice::readyRead, this, [this] {
        m_pcm.append(m_captureIo->readAll());
    });

    m_format = format;
    m_recording = true;
    m_limitTimer->start();
    emit recordingStarted();
    return true;
}

void VoiceInput::stopRecording()
{
    if (!m_recording)
        return;
    m_recording = false;
    m_limitTimer->stop();
    m_tailTimer->start();  // finishCapture() once the tail is in
    emit recordingStopped();
}

void VoiceInput::finishCapture()
{
    m_pcm.append(m_captureIo->readAll());  // still buffered: the last bit of speech
    releaseSource();

    const qreal seconds = m_format.durationForBytes(m_pcm.size()) / 1e6;
    if (seconds < kMinClipSeconds) {
        m_pcm.clear();
        emit recognized(QString());  // too short: nothing to transcribe
        return;
    }
    uploadClip();
}

void VoiceInput::cancelRecording()
{
    m_recording = false;
    m_limitTimer->stop();
    m_tailTimer->stop();
    if (m_source)  // recording, or still in the tail
        releaseSource();
    m_pcm.clear();
    if (m_upload) {
        // Disconnect first: abort() emits finished() synchronously.
        QNetworkReply *reply = std::exchange(m_upload, nullptr);
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}

void VoiceInput::releaseSource()
{
    m_source->stop();
    delete m_source;
    m_source = nullptr;
    m_captureIo = nullptr;
}

// Wrap the captured samples in a canonical 44-byte RIFF/WAVE header for the
// upload. Integer formats are plain PCM (8-bit PCM is unsigned in WAV, like
// QAudioFormat::UInt8); a Float device format needs the IEEE-float tag, or
// the server would decode the samples as 32-bit integers — pure noise.
QByteArray VoiceInput::wrapWav(const QByteArray &pcm, const QAudioFormat &format)
{
    QByteArray out;
    out.reserve(44 + pcm.size());
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    const quint16 formatTag = format.sampleFormat() == QAudioFormat::Float ? 3 : 1;
    const quint16 blockAlign = format.bytesPerFrame();
    const quint32 byteRate = format.sampleRate() * blockAlign;
    ds.writeRawData("RIFF", 4);
    ds << quint32(36 + pcm.size());
    ds.writeRawData("WAVE", 4);
    ds.writeRawData("fmt ", 4);
    ds << quint32(16);             // fmt chunk size
    ds << formatTag;
    ds << quint16(format.channelCount());
    ds << quint32(format.sampleRate());
    ds << byteRate;
    ds << blockAlign;
    ds << quint16(format.bytesPerSample() * 8);
    ds.writeRawData("data", 4);
    ds << quint32(pcm.size());
    out.append(pcm);
    return out;
}

void VoiceInput::uploadClip()
{
    SkvirtSettings::self()->load();

    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("audio/wav"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"file\"; filename=\"voice.wav\""));
    filePart.setBody(wrapWav(m_pcm, m_format));
    multi->append(filePart);
    m_pcm.clear();

    QHttpPart formatPart;
    formatPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QStringLiteral("form-data; name=\"response_format\""));
    formatPart.setBody("json");
    multi->append(formatPart);

    // Sent as is: "auto" makes whisper detect the language, "ru"/"en"/… pin
    // it. Only an empty setting defers to the server's own -l — which is "en"
    // unless the server was started otherwise, garbling any other language.
    const QString language = SkvirtSettings::self()->whisperLanguage().trimmed();
    if (!language.isEmpty()) {
        QHttpPart langPart;
        langPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QStringLiteral("form-data; name=\"language\""));
        langPart.setBody(language.toUtf8());
        multi->append(langPart);
    }

    QNetworkRequest req(endpoint(QStringLiteral("/inference")));
    req.setTransferTimeout(kInferenceTimeoutMs);
    m_upload = m_nam->post(req, multi);
    multi->setParent(m_upload);

    connect(m_upload, &QNetworkReply::finished, this, [this] {
        QNetworkReply *reply = std::exchange(m_upload, nullptr);
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const QJsonObject json = QJsonDocument::fromJson(body).object();

        // No HTTP status at all: nothing answered (refused, timed out, …).
        if (status == 0) {
            emit availabilityChanged(false);
            emit failed(QStringLiteral("whisper-server unreachable (%1)")
                            .arg(reply->errorString()));
            return;
        }
        // The server is there but rejected the clip: 400 for unreadable
        // audio, 500 when inference fails — mostly as {"error": "…"}, but a
        // malformed request gets a plain-text body instead.
        QString error = json.value(QStringLiteral("error")).toString();
        if (status != 200 || !error.isEmpty()) {
            if (error.isEmpty())
                error = QString::fromUtf8(body.left(80)).trimmed();
            emit failed(QStringLiteral("whisper-server: %1")
                            .arg(error.isEmpty() ? QStringLiteral("HTTP %1").arg(status) : error));
            return;
        }

        // whisper-server ends every segment with "\n", which would be typed
        // as Enter (sending a chat message halfway): fold all whitespace into
        // single spaces. Drop its non-speech annotations too ("[BLANK_AUDIO]",
        // "[Music]", …), which are never meant as text.
        static const QRegularExpression annotation(QStringLiteral("\\[[^\\]]*\\]"));
        const QString text = json.value(QStringLiteral("text")).toString()
                                 .remove(annotation).simplified();
        // Silence or noise tends to come back as a lone "." or "…": nothing
        // with a letter or digit in it was said.
        const bool spoken = std::any_of(text.cbegin(), text.cend(),
                                        [](QChar c) { return c.isLetterOrNumber(); });
        emit recognized(spoken ? text : QString());
    });
}
