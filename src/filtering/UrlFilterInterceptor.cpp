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
        // Inject UA-CH (User-Agent Client Hints) headers on Google auth requests.
        // Google's GlifWebSignIn verifies these server-side; Qt does not send them
        // automatically, which causes the "browser not secure" rejection.
        info.setHttpHeader("Sec-CH-UA",
            "\"Google Chrome\";v=\"131\", \"Chromium\";v=\"131\", \"Not_A Brand\";v=\"24\"");
        info.setHttpHeader("Sec-CH-UA-Mobile", "?0");
        info.setHttpHeader("Sec-CH-UA-Platform", "\"Windows\"");
        info.setHttpHeader("Sec-CH-UA-Platform-Version", "\"10.0.0\"");
        info.setHttpHeader("Sec-CH-UA-Full-Version-List",
            "\"Google Chrome\";v=\"131.0.6778.205\", \"Chromium\";v=\"131.0.6778.205\", \"Not_A Brand\";v=\"24.0.0.0\"");

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
