#pragma once

#include <QMainWindow>
#include <QUrl>
#include <QMap>
#include <QMetaObject>
#include <memory>

class BrowserView;
class NavigationManager;
class FilterManager;
class CosmeticFilterManager;
class SettingsManager;
class VideoDownloader;
class SplashScreen;
class QWebEngineProfile;

class QLineEdit;
class QToolButton;
class QPushButton;
class QProgressBar;
class QLabel;
class QTabBar;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // Toolbar / page slots (wired to the active tab only)
    void handleNavigationChanged(const QUrl& url, const QString& title);
    void handleLoadingProgress(int progress);
    void handleFullScreenToggled(bool fullScreen);
    void handleAddressEntered();
    void updateFilterPill();
    void applyTheme();
    void handleInitialLoadFinished(bool success);

    // Tab management slots
    void handleTabChanged(int index);
    void handleTabCloseRequested(int index);

    // Dialog slots
    void openSettingsDialog();
    void openDownloadDialog();
    void openAboutDialog();

private:
    void setupUi();
    void setupShortcuts();

    BrowserView* createBrowserTab(const QUrl& url = QUrl(), bool setAsCurrent = true);
    void openNewTab(const QUrl& url = QUrl());
    BrowserView* currentBrowserView() const;
    NavigationManager* currentNavManager() const;

    void connectTabSignals(BrowserView* view, NavigationManager* nav);
    void disconnectTabSignals();
    void updateTabTitle(BrowserView* view, const QString& title);

    // Subsystems (shared across all tabs)
    std::unique_ptr<FilterManager>        m_filterManager;
    std::unique_ptr<CosmeticFilterManager> m_cosmeticManager;
    std::unique_ptr<SettingsManager>      m_settingsManager;
    std::unique_ptr<VideoDownloader>      m_videoDownloader;
    QWebEngineProfile*                    m_sharedProfile{nullptr};

    // Tab Bar & Content Stack
    QWidget*        m_tabBarContainer{nullptr};
    QTabBar*        m_tabBar{nullptr};
    QToolButton*    m_newTabBtn{nullptr};
    QStackedWidget* m_stackedWidget{nullptr};
    QMap<BrowserView*, NavigationManager*> m_navManagers;

    // Active tab connection handles
    QMetaObject::Connection m_connNavChanged;
    QMetaObject::Connection m_connLoadProgress;
    QMetaObject::Connection m_connFullScreen;

    // Splash overlay
    SplashScreen* m_splashScreen{nullptr};

    // Navigation Toolbar
    QWidget*      m_topBar{nullptr};
    QLabel*       m_brandLogo{nullptr};
    QLabel*       m_brandText{nullptr};
    QToolButton*  m_backBtn{nullptr};
    QToolButton*  m_forwardBtn{nullptr};
    QToolButton*  m_reloadBtn{nullptr};
    QToolButton*  m_homeBtn{nullptr};
    QLineEdit*    m_addressEdit{nullptr};
    QToolButton*  m_shieldBtn{nullptr};
    QPushButton*  m_downloadBtn{nullptr};
    QToolButton*  m_settingsBtn{nullptr};
    QToolButton*  m_aboutBtn{nullptr};
    QProgressBar* m_loadingBar{nullptr};

    bool m_wasMaximizedBeforeFullscreen{false};
    bool m_firstPageLoaded{false};
};
