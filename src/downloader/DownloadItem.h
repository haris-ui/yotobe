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

class DownloadItem : public QObject {
    Q_OBJECT
public:
    explicit DownloadItem(const QUrl& videoUrl, const QString& outputPath, QObject* parent = nullptr);

    QUrl videoUrl() const { return m_videoUrl; }
    QString outputPath() const { return m_outputPath; }
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
    QString m_title;
    int m_progress{0};
    QString m_statusText{"Queued"};
    DownloadState m_state{DownloadState::Queued};
};
