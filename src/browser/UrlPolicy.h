#pragma once

#include <QUrl>
#include <QStringList>

class UrlPolicy {
public:
    UrlPolicy();

    bool isAllowedNavigation(const QUrl& url) const;
    bool isYouTubeDomain(const QUrl& url) const;
    bool isAuthDomain(const QUrl& url) const;

    QUrl normalizeUrl(const QString& input) const;
    static QUrl defaultUrl();

private:
    QStringList m_allowedHosts;
};
