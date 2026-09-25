#include "UrlFilterInterceptor.h"
#include "RuleMatcher.h"
#include "FilterStatistics.h"
#include "UrlPolicy.h"

UrlFilterInterceptor::UrlFilterInterceptor(std::shared_ptr<RuleMatcher> matcher,
                                           std::shared_ptr<FilterStatistics> stats,
                                           QObject* parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_matcher(std::move(matcher))
    , m_stats(std::move(stats))
{
}

void UrlFilterInterceptor::setFilteringEnabled(bool enabled) {
    m_filteringEnabled = enabled;
}

bool UrlFilterInterceptor::isFilteringEnabled() const {
    return m_filteringEnabled;
}

static ResourceTypeFlag mapResourceType(QWebEngineUrlRequestInfo::ResourceType type) {
    switch (type) {
    case QWebEngineUrlRequestInfo::ResourceTypeScript:    return ResourceTypeFlag::Script;
    case QWebEngineUrlRequestInfo::ResourceTypeImage:     return ResourceTypeFlag::Image;
    case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:  return ResourceTypeFlag::Subdocument;
    case QWebEngineUrlRequestInfo::ResourceTypeMedia:     return ResourceTypeFlag::Media;
    case QWebEngineUrlRequestInfo::ResourceTypeXhr:       return ResourceTypeFlag::XHR;
    case QWebEngineUrlRequestInfo::ResourceTypePing:      return ResourceTypeFlag::Ping;
    default:                                              return ResourceTypeFlag::Other;
    }
}

void UrlFilterInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    const QUrl url        = info.requestUrl();
    const QUrl firstParty = info.firstPartyUrl();

    // Reuse UrlPolicy::isAuthDomain() — single authoritative list, no duplication.
    // A static instance is safe here; UrlPolicy is immutable after construction.
    static const UrlPolicy s_policy;
    const bool isAuthContext = s_policy.isAuthDomain(url) || s_policy.isAuthDomain(firstParty);

    if (isAuthContext) {
        // Never block or mutate any request belonging to Google authentication.
        // Leaving headers natural and untampered ensures standard browser compliance.
        if (m_stats) m_stats->recordAllowed(url.toString());
        return;
    }

    if (!m_filteringEnabled) {
        if (m_stats) m_stats->recordAllowed(url.toString());
        return;
    }

    const ResourceTypeFlag resType = mapResourceType(info.resourceType());
    const MatchResult result = m_matcher->evaluate(url, resType, firstParty);

    if (result.shouldBlock) {
        info.block(true);
        if (m_stats) m_stats->recordBlocked(url.toString(), result.matchedRule);
    } else {
        if (m_stats) m_stats->recordAllowed(url.toString());
    }
}
