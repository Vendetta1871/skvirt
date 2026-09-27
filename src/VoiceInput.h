#pragma once

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

class QAudioSource;
class QIODevice;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// Push-to-talk voice input backed by whisper.cpp's whisper-server over HTTP.
// This is a purely runtime dependency: nothing links against whisper.cpp, the
// code just talks to whatever answers on the configured address (default
// http://127.0.0.1:8080, the server's standard port). When no server is
// there, availabilityChanged(false) keeps the UI hint hidden and start/stop
// degrade to a brief error note.
class VoiceInput : public QObject
{
    Q_OBJECT
public:
    explicit VoiceInput(QObject *parent = nullptr);
    ~VoiceInput() override;

    bool recording() const { return m_recording; }

    // Probe GET <server>/health; the result arrives via availabilityChanged.
    void checkAvailability();

    // Start capturing the default microphone. Returns false immediately when
    // there is no usable input device; otherwise recordingStarted() follows.
    bool startRecording();
    // Stop (after a short tail, so the last syllable isn't cut off) and POST
    // the clip to <server>/inference: recordingStopped() fires right away,
    // the transcript arrives via recognized(), problems via failed().
    void stopRecording();
    // Stop and discard (gesture cancelled, panel hidden, …), including an
    // upload already in flight — no signals after.
    void cancelRecording();

signals:
    void availabilityChanged(bool available);
    void recordingStarted();
    // The take is over (released, or the 2-minute cap hit); transcription follows.
    void recordingStopped();
    void recognized(const QString &text);
    void failed(const QString &message);

private:
    void finishCapture();
    void releaseSource();
    void uploadClip();
    static QByteArray wrapWav(const QByteArray &pcm, const QAudioFormat &format);

    QNetworkAccessManager *m_nam;
    QNetworkReply *m_probe = nullptr;
    QNetworkReply *m_upload = nullptr; // transcription request in flight
    QAudioSource *m_source = nullptr;
    QIODevice *m_captureIo = nullptr;  // pull device handed out by m_source
    QByteArray m_pcm;                  // captured samples
    QAudioFormat m_format;             // what m_pcm was captured in
    QTimer *m_limitTimer;              // caps one take at 2 minutes
    QTimer *m_tailTimer;               // keeps capturing briefly after release
    bool m_recording = false;
};
