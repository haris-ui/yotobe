#include "VideoDownloader.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDebug>

VideoDownloader::VideoDownloader(QObject* parent)
    : QObject(parent)
{
    m_ytDlpPath = findYtDlpBinary();
}

VideoDownloader::~VideoDownloader() {
    cancelCurrentDownload();
}

QString VideoDownloader::findYtDlpBinary() {
    // 1. Look in the directory containing Yotobe.exe
    //    QDir::currentPath() is unreliable on Windows (CWD is often the user's
    //    home folder when launched via shortcut). applicationDirPath() is always
    //    the actual exe directory regardless of launch method.
    QString appDir = QCoreApplication::applicationDirPath();
    QString localYtDlp = appDir + "/yt-dlp.exe";
    if (QFileInfo::exists(localYtDlp)) {
        return localYtDlp;
    }

    // 2. Search on system PATH
    QString pathBinary = QStandardPaths::findExecutable("yt-dlp");
    if (!pathBinary.isEmpty()) {
        return pathBinary;
    }

    return QString();
}

bool VideoDownloader::isBackendAvailable() {
    return !findYtDlpBinary().isEmpty();
}

QString VideoDownloader::buildFormatArgs(DownloadFormat format) const {
    switch (format) {
    case DownloadFormat::Video1080p:
        return "bestvideo[height<=1080]+bestaudio/best[height<=1080]/best";
    case DownloadFormat::Video720p:
        return "bestvideo[height<=720]+bestaudio/best[height<=720]/best";
    case DownloadFormat::AudioOnlyMP3:
        return "bestaudio/best";
    case DownloadFormat::BestVideoAudio:
    default:
        return "bestvideo+bestaudio/best";
    }
}

DownloadItem* VideoDownloader::startDownload(const QUrl& url, const QString& destinationFolder, DownloadFormat format) {
    if (m_ytDlpPath.isEmpty()) {
        m_ytDlpPath = findYtDlpBinary();
    }

    DownloadItem* item = new DownloadItem(url, destinationFolder, this);
    m_items.append(item);
    m_queue.enqueue(item);

    processNextInQueue();
    return item;
}

void VideoDownloader::processNextInQueue() {
    if (m_currentProcess != nullptr || m_queue.isEmpty()) {
        return;
    }

    m_currentItem = m_queue.dequeue();
    m_currentItem->setState(DownloadState::Downloading);
    m_currentItem->setStatusText("Starting download...");

    emit downloadStarted(m_currentItem);

    if (m_ytDlpPath.isEmpty()) {
        m_currentItem->setState(DownloadState::Failed);
        m_currentItem->setStatusText("yt-dlp executable not found");
        emit downloadFinished(m_currentItem, false, "yt-dlp not found on system. Please install yt-dlp to enable video downloads.");
        m_currentItem = nullptr;
        processNextInQueue();
        return;
    }

    m_currentProcess = new QProcess(this);
    connect(m_currentProcess, &QProcess::readyReadStandardOutput, this, &VideoDownloader::handleProcessOutput);
    connect(m_currentProcess, &QProcess::readyReadStandardError, this, &VideoDownloader::handleProcessError);
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoDownloader::handleProcessFinished);

    QString outputTemplate = m_currentItem->outputPath() + "/%(title)s.%(ext)s";
    QStringList args;
    args << "--newline"
         << "-o" << outputTemplate
         << "-f" << buildFormatArgs(DownloadFormat::BestVideoAudio)
         << m_currentItem->videoUrl().toString();

    m_currentProcess->start(m_ytDlpPath, args);
}

void VideoDownloader::handleProcessOutput() {
    if (!m_currentProcess || !m_currentItem) return;

    while (m_currentProcess->canReadLine()) {
        QString line = QString::fromUtf8(m_currentProcess->readLine()).trimmed();
        
        // Match yt-dlp progress: "[download]  45.2% of ~12.50MiB at 4.25MiB/s ETA 00:03"
        static QRegularExpression progressRe(R"(\[download\]\s+([\d\.]+)%\s+of\s+([^\s]+)\s+at\s+([^\s]+)\s+ETA\s+([^\s]+))");
        QRegularExpressionMatch match = progressRe.match(line);
        if (match.hasMatch()) {
            double percent = match.captured(1).toDouble();
            QString speed = match.captured(3);
            QString eta = match.captured(4);
            QString status = QString("%1% (%2, ETA %3)").arg(QString::number(percent, 'f', 1), speed, eta);

            m_currentItem->setProgress(static_cast<int>(percent));
            m_currentItem->setStatusText(status);
            emit downloadProgress(m_currentItem, static_cast<int>(percent), status);
        } else if (line.startsWith("[download] Destination: ")) {
            QString dest = line.mid(24);
            m_currentItem->setTitle(QFileInfo(dest).fileName());
        }
    }
}

void VideoDownloader::handleProcessError() {
    if (!m_currentProcess) return;
    QString err = QString::fromUtf8(m_currentProcess->readAllStandardError()).trimmed();
    if (!err.isEmpty()) {
        qDebug() << "yt-dlp stderr:" << err;
    }
}

void VideoDownloader::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_currentItem) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            m_currentItem->setProgress(100);
            m_currentItem->setState(DownloadState::Completed);
            m_currentItem->setStatusText("Complete");
            emit downloadFinished(m_currentItem, true, "Download finished successfully.");
        } else {
            m_currentItem->setState(DownloadState::Failed);
            m_currentItem->setStatusText(QString("Failed (Exit code %1)").arg(exitCode));
            emit downloadFinished(m_currentItem, false, "Download failed.");
        }
    }

    if (m_currentProcess) {
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
    }
    m_currentItem = nullptr;

    processNextInQueue();
}

void VideoDownloader::cancelCurrentDownload() {
    if (m_currentProcess && m_currentProcess->state() != QProcess::NotRunning) {
        m_currentProcess->kill();
        if (m_currentItem) {
            m_currentItem->setState(DownloadState::Cancelled);
            m_currentItem->setStatusText("Cancelled");
        }
    }
}
