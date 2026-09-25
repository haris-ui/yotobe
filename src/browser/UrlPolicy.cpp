#include "UrlPolicy.h"
#include <QUrlQuery>

UrlPolicy::UrlPolicy() {
    m_allowedHosts = {
        "youtube.com",
        "www.youtube.com",
        "m.youtube.com",
        "music.youtube.com",
        "gaming.youtube.com",
        "youtu.be",
        "accounts.youtube.com",
        "google.com",
        "accounts.google.com",
        "myaccount.google.com",
        "consent.google.com",
        "apis.google.com",
        "googleapis.com",
        "gstatic.com",
        "ssl.gstatic.com",
        "googleusercontent.com",
        "googlevideo.com",
        "play.google.com",
        // OAuth redirect CDN endpoints that appear during Google login flow
        "ggpht.com",            // Google avatar CDN (yt3.ggpht.com profile images)
        "ytimg.com",            // YouTube image CDN (i.ytimg.com thumbnails)
        "yt3.ggpht.com",        // YouTube avatar CDN explicit
        "lh3.googleusercontent.com", // Google profile photo CDN after login
    };
}

QUrl UrlPolicy::defaultUrl() {
    return QUrl("https://www.youtube.com/");
}

bool UrlPolicy::isYouTubeDomain(const QUrl& url) const {
    const QString host = url.host().toLower();
    return host == "youtube.com" || host.endsWith(".youtube.com") || host == "youtu.be";
}

bool UrlPolicy::isAuthDomain(const QUrl& url) const {
    const QString host = url.host().toLower();
    return host == "accounts.google.com" ||
           host.endsWith(".accounts.google.com") ||
           host == "myaccount.google.com" ||
           host == "consent.google.com" ||
           host == "accounts.youtube.com" ||
           host == "apis.google.com";
}

bool UrlPolicy::isAllowedNavigation(const QUrl& url) const {
    if (!url.isValid()) return false;
    const QString scheme = url.scheme().toLower();
    if (scheme != "http" && scheme != "https" && scheme != "about") {
        return false;
    }
    if (scheme == "about") return true;

    const QString host = url.host().toLower();
    for (const QString& allowed : m_allowedHosts) {
        if (host == allowed || host.endsWith("." + allowed)) {
            return true;
        }
    }
    return false;
}

QUrl UrlPolicy::normalizeUrl(const QString& input) const {
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return defaultUrl();
    }

    if (trimmed.startsWith("http://") || trimmed.startsWith("https://")) {
        return QUrl(trimmed);
    }

    // If it looks like a search query rather than a URL or domain
    if (!trimmed.contains('.') || trimmed.contains(' ')) {
        QUrl searchUrl("https://www.youtube.com/results");
        QUrlQuery query;
        query.addQueryItem("search_query", trimmed);
        searchUrl.setQuery(query);
        return searchUrl;
    }

    // Default to https
    return QUrl("https://" + trimmed);
}
