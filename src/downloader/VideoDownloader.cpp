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
    // Resolve binary paths once at startup — not on every static call.
    m_ytDlpPath  = findYtDlpBinary();
    m_ffmpegPath = findFfmpegBinary();
}

VideoDownloader::~VideoDownloader() {
    cancelCurrentDownload();
}

QString VideoDownloader::findYtDlpBinary() {
    // 1. Next to Yotobe.exe (preferred — portable deployment)
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString local  = appDir + "/yt-dlp.exe";
    if (QFileInfo::exists(local)) return local;

    // 2. System PATH
    const QString inPath = QStandardPaths::findExecutable("yt-dlp");
    if (!inPath.isEmpty()) return inPath;

    return {};
}

QString VideoDownloader::findFfmpegBinary() {
    // 1. Next to Yotobe.exe
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString local  = appDir + "/ffmpeg.exe";
    if (QFileInfo::exists(local)) return local;

    // 2. System PATH
    const QString inPath = QStandardPaths::findExecutable("ffmpeg");
    if (!inPath.isEmpty()) return inPath;

    // 3. WinGet installation directory
    const QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir wingetDir(localAppData + "/Microsoft/WinGet/Packages");
    if (wingetDir.exists()) {
        const QStringList entries =
            wingetDir.entryList(QStringList() << "*FFmpeg*", QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            QDir sub(wingetDir.filePath(entry));
            for (const QString& s : sub.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                const QString candidate = sub.filePath(s) + "/bin/ffmpeg.exe";
                if (QFileInfo::exists(candidate)) return candidate;
            }
        }
    }

    return {};
}

bool VideoDownloader::isBackendAvailable() {
    // Use cached path — do not re-scan the filesystem every call.
    // Falls back to static scan only when called before a VideoDownloader is constructed
    // (e.g. from DownloadDialog when no instance exists yet).
    return !findYtDlpBinary().isEmpty();
}

bool VideoDownloader::isFfmpegAvailable() {
    return !findFfmpegBinary().isEmpty();
}

QString VideoDownloader::buildFormatString(DownloadFormat format, bool hasFfmpeg) const {
    switch (format) {
    case DownloadFormat::Video1080p:
        return hasFfmpeg
            ? QStringLiteral("bestvideo[height<=1080]+bestaudio/best[height<=1080]/best")
            : QStringLiteral("best[height<=1080][ext=mp4]/best[height<=1080]/best");

    case DownloadFormat::Video720p:
        return hasFfmpeg
            ? QStringLiteral("bestvideo[height<=720]+bestaudio/best[height<=720]/best")
            : QStringLiteral("best[height<=720][ext=mp4]/best[height<=720]/best");

    case DownloadFormat::Video480p:
        return hasFfmpeg
            ? QStringLiteral("bestvideo[height<=480]+bestaudio/best[height<=480]/best")
            : QStringLiteral("best[height<=480][ext=mp4]/best[height<=480]/best");

    case DownloadFormat::AudioOnlyMP3:
        return QStringLiteral("bestaudio/best");

    case DownloadFormat::AudioOnlyM4A:
        return QStringLiteral("bestaudio[ext=m4a]/bestaudio/best");

    case DownloadFormat::BestVideoAudio:
    default:
        return hasFfmpeg
            ? QStringLiteral("bestvideo+bestaudio/best")
            : QStringLiteral("best[ext=mp4]/best");
    }
}

DownloadItem* VideoDownloader::startDownload(const QUrl& url,
                                              const QString& destinationFolder,
                                              DownloadFormat format) {
    // Re-probe if first startup scan failed (e.g. tool installed after launch)
    if (m_ytDlpPath.isEmpty())  m_ytDlpPath  = findYtDlpBinary();
    if (m_ffmpegPath.isEmpty()) m_ffmpegPath = findFfmpegBinary();

    auto* item = new DownloadItem(url, destinationFolder, format, this);
    m_items.append(item);
    m_queue.enqueue(item);
    processNextInQueue();
    return item;
}

void VideoDownloader::processNextInQueue() {
    if (m_currentProcess != nullptr || m_queue.isEmpty()) return;

    m_currentItem = m_queue.dequeue();
    m_currentItem->setState(DownloadState::Downloading);
    m_currentItem->setStatusText("Initializing download...");
    m_lastErrorOutput.clear();

    emit downloadStarted(m_currentItem);

    if (m_ytDlpPath.isEmpty()) {
        m_currentItem->setState(DownloadState::Failed);
        m_currentItem->setStatusText("yt-dlp executable not found");
        emit downloadFinished(m_currentItem, false,
            "yt-dlp not found. Place yt-dlp.exe in the Yotobe directory.");
        m_currentItem = nullptr;
        processNextInQueue();
        return;
    }

    m_currentProcess = new QProcess(this);
    connect(m_currentProcess, &QProcess::readyReadStandardOutput,
            this, &VideoDownloader::handleProcessOutput);
    connect(m_currentProcess, &QProcess::readyReadStandardError,
            this, &VideoDownloader::handleProcessError);
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoDownloader::handleProcessFinished);

    QStringList args;
    args << "--newline" << "--no-playlist" << "--no-mtime" << "--no-warnings";

    const bool hasFfmpeg = !m_ffmpegPath.isEmpty();
    if (hasFfmpeg) {
        args << "--ffmpeg-location" << QFileInfo(m_ffmpegPath).absolutePath();
    }

    const DownloadFormat fmt = m_currentItem->format();
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
        const QString line = QString::fromUtf8(m_currentProcess->readLine()).trimmed();

        if (line.startsWith("[download] Destination: ")) {
            m_currentItem->setTitle(QFileInfo(line.mid(24).trimmed()).fileName());
        } else if (line.contains("has already been downloaded")) {
            static const QRegularExpression alreadyRe(
                R"(\[download\]\s+(.+?)\s+has already been downloaded)");
            const auto m = alreadyRe.match(line);
            if (m.hasMatch()) {
                m_currentItem->setTitle(QFileInfo(m.captured(1)).fileName());
            }
            m_currentItem->setProgress(100);
            m_currentItem->setStatusText("Already downloaded");
            emit downloadProgress(m_currentItem, 100, "Already downloaded");
        } else if (line.startsWith("[Merger]")) {
            m_currentItem->setProgress(99);
            m_currentItem->setStatusText("Merging video and audio...");
            emit downloadProgress(m_currentItem, 99, "Merging video and audio...");
        } else if (line.startsWith("[ExtractAudio]")) {
            m_currentItem->setProgress(99);
            m_currentItem->setStatusText("Converting audio to MP3...");
            emit downloadProgress(m_currentItem, 99, "Converting audio to MP3...");
        } else {
            static const QRegularExpression progressRe(
                R"(\[download\]\s+([\d\.]+)%(?:\s+of\s+~?([^\s]+))?(?:\s+at\s+([^\s]+))?(?:\s+ETA\s+([^\s]+))?)");
            const auto m = progressRe.match(line);
            if (m.hasMatch()) {
                const double percent = m.captured(1).toDouble();
                const QString size   = m.captured(2);
                const QString speed  = m.captured(3);
                const QString eta    = m.captured(4);

                QString status;
                if (!speed.isEmpty() && !eta.isEmpty()) {
                    status = QStringLiteral("%1% of %2 (%3, ETA %4)")
                                 .arg(QString::number(percent, 'f', 1),
                                      size.isEmpty() ? "file" : size,
                                      speed, eta);
                } else if (!speed.isEmpty()) {
                    status = QStringLiteral("%1% (%2)").arg(QString::number(percent, 'f', 1), speed);
                } else {
                    status = QStringLiteral("%1% downloading...").arg(QString::number(percent, 'f', 1));
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
    const QString err = QString::fromUtf8(m_currentProcess->readAllStandardError()).trimmed();
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
            QString errMsg = QStringLiteral("Download failed.");
            if (m_lastErrorOutput.contains("Private video")) {
                errMsg = "This video is private.";
            } else if (m_lastErrorOutput.contains("Sign in to confirm your age")) {
                errMsg = "This video is age-restricted.";
            } else if (m_lastErrorOutput.contains("Video unavailable")) {
                errMsg = "Video is unavailable.";
            } else if (exitCode != 0) {
                errMsg = QStringLiteral("Download failed (Error code %1)").arg(exitCode);
            }
            m_currentItem->setStatusText(errMsg);
            emit downloadFinished(m_currentItem, false, errMsg);
        }
    }

    // Clean up process and advance the queue.
    if (m_currentProcess) {
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
    }
    m_currentItem = nullptr;
    processNextInQueue();
}

void VideoDownloader::cancelCurrentDownload() {
    if (!m_currentProcess || m_currentProcess->state() == QProcess::NotRunning) return;

    // Disconnect finished signal first to prevent handleProcessFinished from
    // double-emitting downloadFinished after we emit it here.
    disconnect(m_currentProcess, nullptr, this, nullptr);
    m_currentProcess->kill();
    m_currentProcess->deleteLater();
    m_currentProcess = nullptr;

    if (m_currentItem) {
        m_currentItem->setState(DownloadState::Cancelled);
        m_currentItem->setStatusText("Cancelled");
        emit downloadFinished(m_currentItem, false, "Download cancelled by user.");
        m_currentItem = nullptr;
    }

    // Advance to next queued download (if any)
    processNextInQueue();
}
