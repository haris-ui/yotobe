#include "FilterStatistics.h"
#include <QDateTime>
#include <QMetaObject>

FilterStatistics::FilterStatistics(QObject* parent)
    : QObject(parent)
{
    // Throttle timer runs on the GUI thread — single-shot, re-armed on each block event.
    // This prevents flooding the main thread with hundreds of statsChanged() signals
    // during a heavy page load.
    m_notifyTimer = new QTimer(this);
    m_notifyTimer->setSingleShot(true);
    m_notifyTimer->setInterval(500);
    connect(m_notifyTimer, &QTimer::timeout, this, &FilterStatistics::statsChanged);
}

qint64 FilterStatistics::totalRequests() const {
    return m_totalRequests.load(std::memory_order_relaxed);
}

qint64 FilterStatistics::allowedRequests() const {
    return m_allowedRequests.load(std::memory_order_relaxed);
}

qint64 FilterStatistics::blockedRequests() const {
    return m_blockedRequests.load(std::memory_order_relaxed);
}

void FilterStatistics::recordAllowed(const QString& /*url*/) {
    // Atomic counter update only — no signal. The shield pill only shows the
    // blocked count, so allowed requests do not need to trigger any UI refresh.
    m_totalRequests.fetch_add(1, std::memory_order_relaxed);
    m_allowedRequests.fetch_add(1, std::memory_order_relaxed);
}

void FilterStatistics::recordBlocked(const QString& url, const QString& ruleMatched) {
    m_totalRequests.fetch_add(1, std::memory_order_relaxed);
    m_blockedRequests.fetch_add(1, std::memory_order_relaxed);

    {
        QMutexLocker locker(&m_logMutex);
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_blockedLogs.prepend(
            QStringLiteral("[%1] Blocked: %2 (Rule: %3)").arg(timestamp, url, ruleMatched));
        // Keep the ring-buffer bounded to 100 entries
        while (m_blockedLogs.size() > 100) {
            m_blockedLogs.removeLast();
        }
    }

    // Schedule a GUI-thread notification via queued invoke so this call,
    // which may originate from the WebEngine IO thread, never touches UI directly.
    // QMetaObject::invokeMethod is thread-safe and will marshal to the object's thread.
    QMetaObject::invokeMethod(this, "scheduleNotify", Qt::QueuedConnection);
}

void FilterStatistics::scheduleNotify() {
    // This slot runs on the GUI thread. Arm the timer only if not already running,
    // which implements the 500 ms throttle.
    if (!m_notifyTimer->isActive()) {
        m_notifyTimer->start();
    }
}

void FilterStatistics::reset() {
    m_totalRequests.store(0, std::memory_order_relaxed);
    m_allowedRequests.store(0, std::memory_order_relaxed);
    m_blockedRequests.store(0, std::memory_order_relaxed);

    {
        QMutexLocker locker(&m_logMutex);
        m_blockedLogs.clear();
    }

    // reset() is always called from the GUI thread (SettingsDialog button click)
    emit statsChanged();
}

QStringList FilterStatistics::recentBlockedLogs(int maxCount) const {
    QMutexLocker locker(&m_logMutex);
    return m_blockedLogs.mid(0, qMin(maxCount, m_blockedLogs.size()));
}
