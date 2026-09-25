#pragma once

#include <QObject>
#include <QProcess>
#include <QQueue>
#include <memory>
#include "DownloadItem.h"

enum class DownloadFormat {
    BestVideoAudio,
    Video1080p,
    Video720p,
    AudioOnlyMP3
};

class VideoDownloader : public QObject {
    Q_OBJECT
public:
    explicit VideoDownloader(QObject* parent = nullptr);
    ~VideoDownloader();

    static QString findYtDlpBinary();
    static bool isBackendAvailable();

    DownloadItem* startDownload(const QUrl& url, const QString& destinationFolder, DownloadFormat format = DownloadFormat::BestVideoAudio);
    void cancelCurrentDownload();

    QList<DownloadItem*> downloadHistory() const { return m_items; }

signals:
    void downloadStarted(DownloadItem* item);
    void downloadProgress(DownloadItem* item, int percent, const QString& speedText);
    void downloadFinished(DownloadItem* item, bool success, const QString& message);

private slots:
    void handleProcessOutput();
    void handleProcessError();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processNextInQueue();

private:
    QString buildFormatArgs(DownloadFormat format) const;

    QList<DownloadItem*> m_items;
    QQueue<DownloadItem*> m_queue;
    QProcess* m_currentProcess{nullptr};
    DownloadItem* m_currentItem{nullptr};
    QString m_ytDlpPath;
};
