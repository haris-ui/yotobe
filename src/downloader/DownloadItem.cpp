#include "DownloadItem.h"

DownloadItem::DownloadItem(const QUrl& videoUrl, const QString& outputPath, DownloadFormat format, QObject* parent)
    : QObject(parent)
    , m_videoUrl(videoUrl)
    , m_outputPath(outputPath)
    , m_format(format)
    , m_title(videoUrl.toString())
{
}

void DownloadItem::setProgress(int p) {
    if (m_progress != p) {
        m_progress = p;
        emit progressChanged(m_progress);
    }
}

void DownloadItem::setStatusText(const QString& status) {
    if (m_statusText != status) {
        m_statusText = status;
        emit statusTextChanged(m_statusText);
    }
}

void DownloadItem::setState(DownloadState s) {
    if (m_state != s) {
        m_state = s;
        emit stateChanged(m_state);
    }
}
