#include "BrowserView.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QDesktopServices>

BrowserView::BrowserView(QWebEngineProfile* profile, QWidget* parent)
    : QWebEngineView(parent)
{
    if (profile) {
        setPage(new QWebEnginePage(profile, this));
    }
    // Enable features required by modern YouTube (HTML5 video, Fullscreen, WebGL, LocalStorage)
    settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);

    connect(page(), &QWebEnginePage::fullScreenRequested,
            this, &BrowserView::handleFullScreenRequested);
}

void BrowserView::applyCustomUserAgent() {
    // Standard modern Chrome desktop User Agent matching Qt 6.8 Chromium 128 core
    const QString ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36";
    if (page() && page()->profile()) {
        page()->profile()->setHttpUserAgent(ua);
    }
}

void BrowserView::handleFullScreenRequested(QWebEngineFullScreenRequest request) {
    request.accept();
    m_isFullScreen = request.toggleOn();
    emit fullScreenToggled(m_isFullScreen);
}

QWebEngineView* BrowserView::createWindow(QWebEnginePage::WebWindowType type) {
    // Tab/window/background-tab requests come from Ctrl+click, middle-click, or window.open().
    // Invoke the factory so MainWindow creates a proper tab and returns its BrowserView.
    // Qt WebEngine then navigates the target URL into that returned view.
    if (m_newTabFactory &&
        (type == QWebEnginePage::WebBrowserTab         ||
         type == QWebEnginePage::WebBrowserBackgroundTab ||
         type == QWebEnginePage::WebBrowserWindow)) {
        if (BrowserView* newView = m_newTabFactory()) {
            return newView;
        }
    }
    // For popup dialogs and unhandled types: stay in the current view.
    return this;
}
