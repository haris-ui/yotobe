#pragma once

#include <QWebEngineView>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineProfile>
#include "UrlPolicy.h"
#include <functional>

class BrowserView : public QWebEngineView {
    Q_OBJECT
public:
    explicit BrowserView(QWebEngineProfile* profile = nullptr, QWidget* parent = nullptr);

    void applyCustomUserAgent();

    UrlPolicy& urlPolicy() { return m_urlPolicy; }
    const UrlPolicy& urlPolicy() const { return m_urlPolicy; }

    bool isFullScreenMode() const { return m_isFullScreen; }

    // Factory invoked by createWindow() when a page requests opening in a new tab/window.
    // The factory creates a new tab in MainWindow and returns its BrowserView.
    // Qt WebEngine navigates the target URL into the returned view.
    using NewTabFactory = std::function<BrowserView*()>;
    void setNewTabFactory(NewTabFactory factory) { m_newTabFactory = std::move(factory); }

signals:
    void fullScreenToggled(bool fullScreen);

protected:
    QWebEngineView* createWindow(QWebEnginePage::WebWindowType type) override;

private slots:
    void handleFullScreenRequested(QWebEngineFullScreenRequest request);

private:
    UrlPolicy m_urlPolicy;
    bool m_isFullScreen{false};
    NewTabFactory m_newTabFactory;
};
