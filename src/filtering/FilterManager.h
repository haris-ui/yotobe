#pragma once

#include <QObject>
#include <memory>
#include "RuleMatcher.h"
#include "FilterStatistics.h"
#include "UrlFilterInterceptor.h"

class FilterManager : public QObject {
    Q_OBJECT
public:
    explicit FilterManager(QObject* parent = nullptr);

    bool initialize();
    bool loadRulesFromFile(const QString& filePath);
    bool loadRulesFromContent(const QString& content);

    std::shared_ptr<RuleMatcher> matcher() const { return m_matcher; }
    std::shared_ptr<FilterStatistics> statistics() const { return m_statistics; }
    UrlFilterInterceptor* interceptor() const { return m_interceptor; }

    void setFilteringEnabled(bool enabled);
    bool isFilteringEnabled() const;

    int totalRulesLoaded() const;

signals:
    void rulesUpdated(int totalRules);

private:
    std::shared_ptr<RuleMatcher> m_matcher;
    std::shared_ptr<FilterStatistics> m_statistics;
    UrlFilterInterceptor* m_interceptor{nullptr};
};
