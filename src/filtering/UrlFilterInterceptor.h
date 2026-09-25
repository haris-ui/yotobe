#pragma once

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>
#include <memory>

class RuleMatcher;
class FilterStatistics;

class UrlFilterInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit UrlFilterInterceptor(std::shared_ptr<RuleMatcher> matcher,
                                  std::shared_ptr<FilterStatistics> stats,
                                  QObject* parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo& info) override;

    void setFilteringEnabled(bool enabled);
    bool isFilteringEnabled() const;

private:
    std::shared_ptr<RuleMatcher> m_matcher;
    std::shared_ptr<FilterStatistics> m_stats;
    bool m_filteringEnabled{true};
};
