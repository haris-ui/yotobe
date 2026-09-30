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

namespace {
    // Comprehensive UA-CH headers matching Chrome 131 on Windows 10
    // These are required for Google's GlifWebSignIn to accept the browser as secure
    void injectAuthHeaders(QWebEngineUrlRequestInfo& info) {
        // Core UA-CH headers (required by Google sign-in)
        info.setHttpHeader("Sec-CH-UA",
            "\"Google Chrome\";v=\"131\", \"Chromium\";v=\"131\", \"Not_A Brand\";v=\"24\"");
        info.setHttpHeader("Sec-CH-UA-Mobile", "?0");
        info.setHttpHeader("Sec-CH-UA-Platform", "\"Windows\"");
        info.setHttpHeader("Sec-CH-UA-Platform-Version", "\"15.0.0\"");  // Windows 10/11
        
        // Full version list (critical for GlifWebSignIn verification)
        info.setHttpHeader("Sec-CH-UA-Full-Version-List",
            "\"Google Chrome\";v=\"131.0.6778.205\", \"Chromium\";v=\"131.0.6778.205\", \"Not_A Brand\";v=\"24.0.0.0\"");
        
        // Architecture and model hints (improves compatibility)
        info.setHttpHeader("Sec-CH-UA-Arch", "\"x86\"");
        info.setHttpHeader("Sec-CH-UA-Model", "\"\"");
        info.setHttpHeader("Sec-CH-UA-Bitness", "\"64\"");
        
        // WoW64 hint (Windows on Windows 64-bit)
        info.setHttpHeader("Sec-CH-UA-WoW64", "?0");
        
        // Form factor
        info.setHttpHeader("Sec-CH-UA-Form-Factors", "\"Desktop\"");
    }
    
    // Check if URL is a Google OAuth/authentication endpoint
    bool isGoogleAuthEndpoint(const QUrl& url) {
        const QString host = url.host().toLower();
        const QString path = url.path().toLower();
        
        // Core auth domains
        if (host == "accounts.google.com" || host.endsWith(".accounts.google.com") ||
            host == "accounts.youtube.com" || host.endsWith(".accounts.youtube.com") ||
            host == "myaccount.google.com" || host.endsWith(".myaccount.google.com") ||
            host == "apis.google.com" || host.endsWith(".apis.google.com") ||
            host == "consent.google.com" || host.endsWith(".consent.google.com") ||
            host == "gds.google.com" || host == "passkeys.google.com") {
            return true;
        }
        
        // OAuth authorization endpoints
        if ((host == "accounts.google.com" || host.endsWith(".google.com")) &&
            (path.contains("/signin") || path.contains("/oauth") || path.contains("/auth") ||
             path.contains("/signin/v2") || path.contains("/login") || path.contains("/identifier"))) {
            return true;
        }
        
        return false;
    }
}

void UrlFilterInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    const QUrl url        = info.requestUrl();
    const QUrl firstParty = info.firstPartyUrl();

    // Reuse UrlPolicy::isAuthDomain() — single authoritative list, no duplication.
    // A static instance is safe here; UrlPolicy is immutable after construction.
    static const UrlPolicy s_policy;
    const bool isAuthContext = s_policy.isAuthDomain(url) || s_policy.isAuthDomain(firstParty);
    const bool isGoogleAuthEndpointUrl = isGoogleAuthEndpoint(url) || isGoogleAuthEndpoint(firstParty);

    if (isAuthContext || isGoogleAuthEndpointUrl) {
        // Inject comprehensive UA-CH headers on Google auth requests
        injectAuthHeaders(info);
        
        // Also inject a proper User-Agent to match the UA-CH
        info.setHttpHeader("User-Agent",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36");

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
