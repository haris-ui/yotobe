#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

enum class DownloadState {
    Queued,
    Downloading,
    Completed,
    Failed,
    Cancelled
};

enum class DownloadFormat {
    BestVideoAudio,
    Video1080p,
    Video720p,
    Video480p,
    AudioOnlyMP3,
    AudioOnlyM4A
};

class DownloadItem : public QObject {
    Q_OBJECT
public:
    explicit DownloadItem(const QUrl& videoUrl, const QString& outputPath, DownloadFormat format = DownloadFormat::BestVideoAudio, QObject* parent = nullptr);

    QUrl videoUrl() const { return m_videoUrl; }
    QString outputPath() const { return m_outputPath; }
    DownloadFormat format() const { return m_format; }
    void setFormat(DownloadFormat f) { m_format = f; }

    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }

    int progress() const { return m_progress; }
    void setProgress(int p);

    QString statusText() const { return m_statusText; }
    void setStatusText(const QString& status);

    DownloadState state() const { return m_state; }
    void setState(DownloadState s);

signals:
    void progressChanged(int progress);
    void stateChanged(DownloadState state);
    void statusTextChanged(const QString& text);

private:
    QUrl m_videoUrl;
    QString m_outputPath;
    DownloadFormat m_format{DownloadFormat::BestVideoAudio};
    QString m_title;
    int m_progress{0};
    QString m_statusText{"Queued"};
    DownloadState m_state{DownloadState::Queued};
};
