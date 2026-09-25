#include "FilterStatistics.h"
#include <QDateTime>

FilterStatistics::FilterStatistics(QObject* parent)
    : QObject(parent)
{
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
    m_totalRequests.fetch_add(1, std::memory_order_relaxed);
    m_allowedRequests.fetch_add(1, std::memory_order_relaxed);
    emit statsChanged();
}

void FilterStatistics::recordBlocked(const QString& url, const QString& ruleMatched) {
    m_totalRequests.fetch_add(1, std::memory_order_relaxed);
    m_blockedRequests.fetch_add(1, std::memory_order_relaxed);

    {
        QMutexLocker locker(&m_logMutex);
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_blockedLogs.prepend(QString("[%1] Blocked: %2 (Rule: %3)").arg(timestamp, url, ruleMatched));
        while (m_blockedLogs.size() > 100) {
            m_blockedLogs.removeLast();
        }
    }

    emit statsChanged();
}

void FilterStatistics::reset() {
    m_totalRequests.store(0, std::memory_order_relaxed);
    m_allowedRequests.store(0, std::memory_order_relaxed);
    m_blockedRequests.store(0, std::memory_order_relaxed);

    {
        QMutexLocker locker(&m_logMutex);
        m_blockedLogs.clear();
    }

    emit statsChanged();
}

QStringList FilterStatistics::recentBlockedLogs(int maxCount) const {
    QMutexLocker locker(&m_logMutex);
    int count = qMin(maxCount, m_blockedLogs.size());
    return m_blockedLogs.mid(0, count);
}
