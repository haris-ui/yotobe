#include "BrowserView.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QDesktopServices>

BrowserView::BrowserView(QWidget* parent)
    : QWebEngineView(parent)
{
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

    // Inject stealth script into profile to ensure Google accounts authentication passes secure browser checks
    QWebEngineScript stealthScript;
    stealthScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
    stealthScript.setWorldId(QWebEngineScript::MainWorld);
    stealthScript.setRunsOnSubFrames(true);
    stealthScript.setName("YotobeStealthScript");
    stealthScript.setSourceCode(QString::fromUtf8(R"(
        (function() {
            'use strict';

            // --- LAYER 1: Always mask automation flag (runs on every page) ---
            try {
                Object.defineProperty(navigator, 'webdriver', {
                    get: () => undefined,
                    configurable: true
                });
            } catch (e) {}

            // --- LAYER 2: Full Firefox identity spoofing on Google auth domains ---
            // CRITICAL: We must also delete window.chrome. If we claim Firefox UA
            // but leave window.chrome alive, Google detects the inconsistency and
            // treats it as a malicious spoof — which is WORSE than no spoofing.
            const h = window.location.hostname;
            const isAuthPage = h && (
                h === 'accounts.google.com' ||
                h.endsWith('.accounts.google.com') ||
                h === 'consent.google.com' ||
                h === 'accounts.youtube.com' ||
                h === 'myaccount.google.com'
            );

            if (isAuthPage) {
                const FF_UA = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:135.0) Gecko/20100101 Firefox/135.0';

                // Override all UA-related navigator properties
                try { Object.defineProperty(navigator, 'userAgent',    { get: () => FF_UA,       configurable: true }); } catch(e) {}
                try { Object.defineProperty(navigator, 'appVersion',   { get: () => '5.0 (Windows)', configurable: true }); } catch(e) {}
                try { Object.defineProperty(navigator, 'appName',      { get: () => 'Netscape',  configurable: true }); } catch(e) {}

                // vendor: Chrome returns 'Google Inc.', Firefox returns '' — mismatch exposes engine
                try { Object.defineProperty(navigator, 'vendor',       { get: () => '',          configurable: true }); } catch(e) {}

                // Remove Client Hints JS API entirely (Chromium-only, Firefox has none)
                try { Object.defineProperty(navigator, 'userAgentData', { get: () => undefined,  configurable: true }); } catch(e) {}

                // Remove plugin list — Chrome has a default plugin list, Firefox has none
                try { Object.defineProperty(navigator, 'plugins',  { get: () => Object.create(PluginArray.prototype), configurable: true }); } catch(e) {}
                try { Object.defineProperty(navigator, 'mimeTypes', { get: () => Object.create(MimeTypeArray.prototype), configurable: true }); } catch(e) {}

                // *** THE CRITICAL FIX ***
                // window.chrome is a Chrome/Chromium-exclusive object.
                // Presence of window.chrome while claiming Firefox UA is the primary
                // signal Google uses to flag embedded-WebView sign-in attempts.
                // We must remove it entirely to be consistent with our Firefox UA claim.
                try {
                    Object.defineProperty(window, 'chrome', {
                        get: () => undefined,
                        set: () => {},
                        configurable: true,
                        enumerable: false
                    });
                } catch(e) {
                    try { delete window.chrome; } catch(e2) {}
                }
            }
        })();
    )"));
    if (page()->profile()->scripts()->find("YotobeStealthScript").isEmpty()) {
        page()->profile()->scripts()->insert(stealthScript);
    }

    // Dynamic User-Agent switching between YouTube and Google Accounts
    connect(this, &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
        if (m_urlPolicy.isAuthDomain(url)) {
            // Firefox User-Agent on Google Accounts avoids embedded Chromium rejection
            page()->profile()->setHttpUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:135.0) Gecko/20100101 Firefox/135.0");
        } else if (m_urlPolicy.isYouTubeDomain(url)) {
            // Chrome User-Agent on YouTube enables 4K/60fps and modern desktop layout
            applyCustomUserAgent();
        }
    });
}

void BrowserView::setProfileStoragePath(const QString& storagePath) {
    QWebEngineProfile* prof = page()->profile();
    prof->setPersistentStoragePath(storagePath);
    prof->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    prof->setCachePath(storagePath + "/cache");
}

void BrowserView::applyCustomUserAgent() {
    // Standard modern Chrome desktop User Agent matching Qt 6.8 Chromium 128 core
    QString ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36";
    page()->profile()->setHttpUserAgent(ua);
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
