#pragma once

#include <QObject>
#include <QUrl>

class BrowserView;

class NavigationManager : public QObject {
    Q_OBJECT
public:
    explicit NavigationManager(BrowserView* browserView, QObject* parent = nullptr);

    void load(const QUrl& url);
    void navigateToInput(const QString& userInput);
    void goBack();
    void goForward();
    void reload();
    void goHome();

    bool canGoBack() const;
    bool canGoForward() const;

signals:
    void navigationChanged(const QUrl& currentUrl, const QString& title);
    void loadingProgress(int progress);
    void externalNavigationBlocked(const QUrl& url);

private slots:
    void handleUrlChanged(const QUrl& url);
    void handleTitleChanged(const QString& title);
    void handleLoadProgress(int progress);

private:
    BrowserView* m_browserView{nullptr};
    bool m_handlingUrlChange{false}; // Re-entrancy guard for handleUrlChanged
};
