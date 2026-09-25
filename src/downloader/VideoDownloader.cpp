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
    m_ffmpegPath = findFfmpegBinary();
}

VideoDownloader::~VideoDownloader() {
    cancelCurrentDownload();
}

QString VideoDownloader::findYtDlpBinary() {
    // 1. Check next to Yotobe.exe
    QString appDir = QCoreApplication::applicationDirPath();
    QString localYtDlp = appDir + "/yt-dlp.exe";
    if (QFileInfo::exists(localYtDlp)) {
        return localYtDlp;
    }

    // 2. Check system PATH
    QString pathBinary = QStandardPaths::findExecutable("yt-dlp");
    if (!pathBinary.isEmpty()) {
        return pathBinary;
    }

    return QString();
}

QString VideoDownloader::findFfmpegBinary() {
    // 1. Check next to Yotobe.exe
    QString appDir = QCoreApplication::applicationDirPath();
    QString localFfmpeg = appDir + "/ffmpeg.exe";
    if (QFileInfo::exists(localFfmpeg)) {
        return localFfmpeg;
    }

    // 2. Check system PATH
    QString pathBinary = QStandardPaths::findExecutable("ffmpeg");
    if (!pathBinary.isEmpty()) {
        return pathBinary;
    }

    // 3. Check WinGet installation directory on Windows
    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir wingetDir(localAppData + "/Microsoft/WinGet/Packages");
    if (wingetDir.exists()) {
        QStringList entries = wingetDir.entryList(QStringList() << "*FFmpeg*", QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            QDir sub(wingetDir.filePath(entry));
            QStringList subDirs = sub.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& s : subDirs) {
                QString candidate = sub.filePath(s) + "/bin/ffmpeg.exe";
                if (QFileInfo::exists(candidate)) {
                    return candidate;
                }
            }
        }
    }

    return QString();
}

bool VideoDownloader::isBackendAvailable() {
    return !findYtDlpBinary().isEmpty();
}

bool VideoDownloader::isFfmpegAvailable() {
    return !findFfmpegBinary().isEmpty();
}

QString VideoDownloader::buildFormatString(DownloadFormat format, bool hasFfmpeg) const {
    switch (format) {
    case DownloadFormat::Video1080p:
        if (hasFfmpeg) {
            return "bestvideo[height<=1080]+bestaudio/best[height<=1080]/best";
        }
        return "best[height<=1080][ext=mp4]/best[height<=1080]/best";

    case DownloadFormat::Video720p:
        if (hasFfmpeg) {
            return "bestvideo[height<=720]+bestaudio/best[height<=720]/best";
        }
        return "best[height<=720][ext=mp4]/best[height<=720]/best";

    case DownloadFormat::Video480p:
        if (hasFfmpeg) {
            return "bestvideo[height<=480]+bestaudio/best[height<=480]/best";
        }
        return "best[height<=480][ext=mp4]/best[height<=480]/best";

    case DownloadFormat::AudioOnlyMP3:
        return "bestaudio/best";

    case DownloadFormat::AudioOnlyM4A:
        return "bestaudio[ext=m4a]/bestaudio/best";

    case DownloadFormat::BestVideoAudio:
    default:
        if (hasFfmpeg) {
            return "bestvideo+bestaudio/best";
        }
        return "best[ext=mp4]/best";
    }
}

DownloadItem* VideoDownloader::startDownload(const QUrl& url, const QString& destinationFolder, DownloadFormat format) {
    if (m_ytDlpPath.isEmpty()) {
        m_ytDlpPath = findYtDlpBinary();
    }
    if (m_ffmpegPath.isEmpty()) {
        m_ffmpegPath = findFfmpegBinary();
    }

    DownloadItem* item = new DownloadItem(url, destinationFolder, format, this);
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
    m_currentItem->setStatusText("Initializing download...");
    m_lastErrorOutput.clear();

    emit downloadStarted(m_currentItem);

    if (m_ytDlpPath.isEmpty()) {
        m_currentItem->setState(DownloadState::Failed);
        m_currentItem->setStatusText("yt-dlp executable not found");
        emit downloadFinished(m_currentItem, false, "yt-dlp not found on system. Place yt-dlp.exe in the Yotobe directory.");
        m_currentItem = nullptr;
        processNextInQueue();
        return;
    }

    m_currentProcess = new QProcess(this);
    connect(m_currentProcess, &QProcess::readyReadStandardOutput, this, &VideoDownloader::handleProcessOutput);
    connect(m_currentProcess, &QProcess::readyReadStandardError,  this, &VideoDownloader::handleProcessError);
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoDownloader::handleProcessFinished);

    QStringList args;
    args << "--newline"
         << "--no-playlist"
         << "--no-mtime"
         << "--no-warnings";

    bool hasFfmpeg = !m_ffmpegPath.isEmpty();
    if (hasFfmpeg) {
        QString ffmpegDir = QFileInfo(m_ffmpegPath).absolutePath();
        args << "--ffmpeg-location" << ffmpegDir;
    }

    // Format selection & post-processing
    DownloadFormat fmt = m_currentItem->format();
    if (fmt == DownloadFormat::AudioOnlyMP3 && hasFfmpeg) {
        args << "-x" << "--audio-format" << "mp3" << "--audio-quality" << "0";
    } else if (hasFfmpeg && fmt != DownloadFormat::AudioOnlyM4A) {
        args << "--merge-output-format" << "mp4";
    }

    args << "-f" << buildFormatString(fmt, hasFfmpeg);
    args << "-o" << (m_currentItem->outputPath() + "/%(title)s.%(ext)s");
    args << m_currentItem->videoUrl().toString();

    m_currentProcess->start(m_ytDlpPath, args);
}

void VideoDownloader::handleProcessOutput() {
    if (!m_currentProcess || !m_currentItem) return;

    while (m_currentProcess->canReadLine()) {
        QString line = QString::fromUtf8(m_currentProcess->readLine()).trimmed();

        // Match destination path to determine title
        if (line.startsWith("[download] Destination: ")) {
            QString dest = line.mid(24).trimmed();
            m_currentItem->setTitle(QFileInfo(dest).fileName());
        }
        else if (line.contains("has already been downloaded")) {
            QRegularExpression alreadyRe(R"(\[download\]\s+(.+?)\s+has already been downloaded)");
            QRegularExpressionMatch alreadyMatch = alreadyRe.match(line);
            if (alreadyMatch.hasMatch()) {
                m_currentItem->setTitle(QFileInfo(alreadyMatch.captured(1)).fileName());
            }
            m_currentItem->setProgress(100);
            m_currentItem->setStatusText("Already downloaded");
            emit downloadProgress(m_currentItem, 100, "Already downloaded");
        }
        else if (line.startsWith("[Merger]")) {
            m_currentItem->setProgress(99);
            m_currentItem->setStatusText("Merging video and audio...");
            emit downloadProgress(m_currentItem, 99, "Merging video and audio...");
        }
        else if (line.startsWith("[ExtractAudio]")) {
            m_currentItem->setProgress(99);
            m_currentItem->setStatusText("Converting audio to MP3...");
            emit downloadProgress(m_currentItem, 99, "Converting audio to MP3...");
        }
        else {
            // Flexible progress regex matching percentage with optional speed and ETA
            static QRegularExpression progressRe(R"(\[download\]\s+([\d\.]+)%(?:\s+of\s+~?([^\s]+))?(?:\s+at\s+([^\s]+))?(?:\s+ETA\s+([^\s]+))?)");
            QRegularExpressionMatch match = progressRe.match(line);
            if (match.hasMatch()) {
                double percent = match.captured(1).toDouble();
                QString size = match.captured(2);
                QString speed = match.captured(3);
                QString eta = match.captured(4);

                QString status;
                if (!speed.isEmpty() && !eta.isEmpty()) {
                    status = QString("%1% of %2 (%3, ETA %4)")
                                 .arg(QString::number(percent, 'f', 1),
                                      size.isEmpty() ? "file" : size,
                                      speed, eta);
                } else if (!speed.isEmpty()) {
                    status = QString("%1% (%2)").arg(QString::number(percent, 'f', 1), speed);
                } else {
                    status = QString("%1% downloading...").arg(QString::number(percent, 'f', 1));
                }

                m_currentItem->setProgress(static_cast<int>(percent));
                m_currentItem->setStatusText(status);
                emit downloadProgress(m_currentItem, static_cast<int>(percent), status);
            }
        }
    }
}

void VideoDownloader::handleProcessError() {
    if (!m_currentProcess) return;
    QString err = QString::fromUtf8(m_currentProcess->readAllStandardError()).trimmed();
    if (!err.isEmpty()) {
        m_lastErrorOutput = err;
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
            QString errMsg = "Download failed.";
            if (m_lastErrorOutput.contains("Private video")) {
                errMsg = "This video is private.";
            } else if (m_lastErrorOutput.contains("Sign in to confirm your age")) {
                errMsg = "This video is age-restricted.";
            } else if (m_lastErrorOutput.contains("Video unavailable")) {
                errMsg = "Video is unavailable.";
            } else if (exitCode != 0) {
                errMsg = QString("Download failed (Error code %1)").arg(exitCode);
            }
            m_currentItem->setStatusText(errMsg);
            emit downloadFinished(m_currentItem, false, errMsg);
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
            emit downloadFinished(m_currentItem, false, "Download cancelled by user.");
        }
    }
}
