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
        // Enforce Firefox User-Agent at the HTTP request level for Google auth pages.
        static const QByteArray s_firefoxUa =
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:135.0) Gecko/20100101 Firefox/135.0";
        info.setHttpHeader("User-Agent", s_firefoxUa);

        // Strip all Chromium Client Hints headers — Firefox sends none of these.
        static const QByteArray s_empty = " ";
        info.setHttpHeader("sec-ch-ua",                   s_empty);
        info.setHttpHeader("sec-ch-ua-mobile",            s_empty);
        info.setHttpHeader("sec-ch-ua-platform",          s_empty);
        info.setHttpHeader("sec-ch-ua-full-version",      s_empty);
        info.setHttpHeader("sec-ch-ua-full-version-list", s_empty);
        info.setHttpHeader("sec-ch-ua-arch",              s_empty);
        info.setHttpHeader("sec-ch-ua-bitness",           s_empty);
        info.setHttpHeader("sec-ch-ua-wow64",             s_empty);
        info.setHttpHeader("sec-ch-ua-model",             s_empty);

        // Match Firefox Accept header exactly
        info.setHttpHeader("Accept",
            "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");

        // Never block any auth-context request
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
