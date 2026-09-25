#include "UrlFilterInterceptor.h"
#include "RuleMatcher.h"
#include "FilterStatistics.h"

UrlFilterInterceptor::UrlFilterInterceptor(std::shared_ptr<RuleMatcher> matcher,
                                           std::shared_ptr<FilterStatistics> stats,
                                           QObject* parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_matcher(matcher)
    , m_stats(stats)
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
    case QWebEngineUrlRequestInfo::ResourceTypeScript:
        return ResourceTypeFlag::Script;
    case QWebEngineUrlRequestInfo::ResourceTypeImage:
        return ResourceTypeFlag::Image;
    case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
        return ResourceTypeFlag::Subdocument;
    case QWebEngineUrlRequestInfo::ResourceTypeMedia:
        return ResourceTypeFlag::Media;
    case QWebEngineUrlRequestInfo::ResourceTypeXhr:
        return ResourceTypeFlag::XHR;
    case QWebEngineUrlRequestInfo::ResourceTypePing:
        return ResourceTypeFlag::Ping;
    default:
        return ResourceTypeFlag::Other;
    }
}

void UrlFilterInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    QUrl url = info.requestUrl();
    QUrl firstParty = info.firstPartyUrl();

    QString host = url.host().toLower();
    QString firstHost = firstParty.host().toLower();

    // Check if this request is part of Google authentication
    bool isAuthContext = (firstHost == "accounts.google.com" ||
                          firstHost.endsWith(".accounts.google.com") ||
                          firstHost == "myaccount.google.com" ||
                          firstHost == "consent.google.com" ||
                          firstHost == "accounts.youtube.com" ||
                          host == "accounts.google.com" ||
                          host.endsWith(".accounts.google.com") ||
                          host == "myaccount.google.com" ||
                          host == "consent.google.com" ||
                          host == "accounts.youtube.com");

    if (isAuthContext) {
        // Enforce Firefox User Agent at HTTP request level for Google auth pages
        static const QByteArray s_firefoxUa =
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:135.0) Gecko/20100101 Firefox/135.0";
        info.setHttpHeader("User-Agent", s_firefoxUa);

        // Strip ALL Chromium Client Hints headers.
        // Setting to "" sends a blank header — use a single space to suppress completely.
        // Firefox does not send any sec-ch-ua headers, so their presence exposes Chromium.
        static const QByteArray s_empty = " ";
        info.setHttpHeader("sec-ch-ua",                  s_empty);
        info.setHttpHeader("sec-ch-ua-mobile",           s_empty);
        info.setHttpHeader("sec-ch-ua-platform",         s_empty);
        info.setHttpHeader("sec-ch-ua-full-version",     s_empty);
        info.setHttpHeader("sec-ch-ua-full-version-list", s_empty);
        info.setHttpHeader("sec-ch-ua-arch",             s_empty);
        info.setHttpHeader("sec-ch-ua-bitness",          s_empty);
        info.setHttpHeader("sec-ch-ua-wow64",            s_empty);
        info.setHttpHeader("sec-ch-ua-model",            s_empty);

        // Force Accept header to match Firefox (Chromium's differs subtly)
        info.setHttpHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");

        // Never block any auth or risk-verification requests
        if (m_stats) m_stats->recordAllowed(url.toString());
        return;
    }

    if (!m_filteringEnabled) {
        if (m_stats) m_stats->recordAllowed(url.toString());
        return;
    }

    ResourceTypeFlag resType = mapResourceType(info.resourceType());
    MatchResult result = m_matcher->evaluate(url, resType, firstParty);

    if (result.shouldBlock) {
        info.block(true);
        if (m_stats) {
            m_stats->recordBlocked(url.toString(), result.matchedRule);
        }
    } else {
        if (m_stats) {
            m_stats->recordAllowed(url.toString());
        }
    }
}
