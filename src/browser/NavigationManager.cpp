#include "NavigationManager.h"
#include "BrowserView.h"
#include <QWebEngineHistory>
#include <QDesktopServices>

NavigationManager::NavigationManager(BrowserView* browserView, QObject* parent)
    : QObject(parent)
    , m_browserView(browserView)
{
    if (m_browserView) {
        connect(m_browserView, &QWebEngineView::urlChanged,
                this, &NavigationManager::handleUrlChanged);
        connect(m_browserView, &QWebEngineView::titleChanged,
                this, &NavigationManager::handleTitleChanged);
        connect(m_browserView, &QWebEngineView::loadProgress,
                this, &NavigationManager::handleLoadProgress);
    }
}

void NavigationManager::load(const QUrl& url) {
    if (!m_browserView) return;

    if (!m_browserView->urlPolicy().isAllowedNavigation(url)) {
        emit externalNavigationBlocked(url);
        QDesktopServices::openUrl(url);
        return;
    }

    m_browserView->load(url);
}

void NavigationManager::navigateToInput(const QString& userInput) {
    if (!m_browserView) return;
    QUrl targetUrl = m_browserView->urlPolicy().normalizeUrl(userInput);
    load(targetUrl);
}

void NavigationManager::goBack() {
    if (m_browserView && m_browserView->history()->canGoBack()) {
        m_browserView->back();
    }
}

void NavigationManager::goForward() {
    if (m_browserView && m_browserView->history()->canGoForward()) {
        m_browserView->forward();
    }
}

void NavigationManager::reload() {
    if (m_browserView) {
        m_browserView->reload();
    }
}

void NavigationManager::goHome() {
    load(UrlPolicy::defaultUrl());
}

bool NavigationManager::canGoBack() const {
    return m_browserView && m_browserView->history()->canGoBack();
}

bool NavigationManager::canGoForward() const {
    return m_browserView && m_browserView->history()->canGoForward();
}

void NavigationManager::handleUrlChanged(const QUrl& url) {
    if (!m_browserView) return;

    // Re-entrancy guard: back() called below can itself trigger urlChanged.
    // Without this guard, a sequence of non-whitelisted redirect URLs in
    // history causes a cascade of back() calls that exhausts the history stack.
    if (m_handlingUrlChange) return;
    m_handlingUrlChange = true;

    // Check if user navigated outside YouTube domains
    if (!m_browserView->urlPolicy().isAllowedNavigation(url)) {
        emit externalNavigationBlocked(url);
        QDesktopServices::openUrl(url);
        m_browserView->back();
        m_handlingUrlChange = false;
        return;
    }

    m_handlingUrlChange = false;
    emit navigationChanged(url, m_browserView->title());
}

void NavigationManager::handleTitleChanged(const QString& title) {
    if (m_browserView) {
        emit navigationChanged(m_browserView->url(), title);
    }
}

void NavigationManager::handleLoadProgress(int progress) {
    emit loadingProgress(progress);
}
