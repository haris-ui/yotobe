#pragma once

#include <QObject>
#include <atomic>
#include <QStringList>
#include <QMutex>

class FilterStatistics : public QObject {
    Q_OBJECT
    Q_PROPERTY(qint64 totalRequests READ totalRequests NOTIFY statsChanged)
    Q_PROPERTY(qint64 allowedRequests READ allowedRequests NOTIFY statsChanged)
    Q_PROPERTY(qint64 blockedRequests READ blockedRequests NOTIFY statsChanged)

public:
    explicit FilterStatistics(QObject* parent = nullptr);

    qint64 totalRequests() const;
    qint64 allowedRequests() const;
    qint64 blockedRequests() const;

    void recordAllowed(const QString& url);
    void recordBlocked(const QString& url, const QString& ruleMatched);
    void reset();

    QStringList recentBlockedLogs(int maxCount = 50) const;

signals:
    void statsChanged();

private:
    std::atomic<qint64> m_totalRequests{0};
    std::atomic<qint64> m_allowedRequests{0};
    std::atomic<qint64> m_blockedRequests{0};

    mutable QMutex m_logMutex;
    QStringList m_blockedLogs;
};
