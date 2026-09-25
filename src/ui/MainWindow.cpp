#include "MainWindow.h"
#include "BrowserView.h"
#include "NavigationManager.h"
#include "FilterManager.h"
#include "FilterStatistics.h"
#include "CosmeticFilterManager.h"
#include "SettingsManager.h"
#include "VideoDownloader.h"
#include "SettingsDialog.h"
#include "DownloadDialog.h"
#include "SplashScreen.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QShortcut>
#include <QKeySequence>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QTabBar>
#include <QStackedWidget>
#include <QWebEnginePage>
#include <QWebEngineProfile>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_filterManager(std::make_unique<FilterManager>(this))
    , m_settingsManager(std::make_unique<SettingsManager>(this))
    , m_videoDownloader(std::make_unique<VideoDownloader>(this))
{
    setWindowTitle("Yotobe");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    resize(1340, 860);

    // 1. Boot filter engine
    m_filterManager->initialize();
    m_filterManager->setFilteringEnabled(m_settingsManager->isFilteringEnabled());

    // 2. Build UI layout and shortcuts
    setupUi();
    setupShortcuts();
    applyTheme();

    // 3. Connect filter & settings signals
    if (m_filterManager->statistics()) {
        connect(m_filterManager->statistics().get(), &FilterStatistics::statsChanged,
                this, &MainWindow::updateFilterPill);
    }
    connect(m_settingsManager.get(), &SettingsManager::themeChanged,
            this, &MainWindow::applyTheme);
    connect(m_settingsManager.get(), &SettingsManager::filteringSettingChanged,
            [this](bool enabled) {
                m_filterManager->setFilteringEnabled(enabled);
                updateFilterPill();
            });
    connect(m_settingsManager.get(), &SettingsManager::cosmeticFilteringSettingChanged,
            [this](bool enabled) {
                if (m_cosmeticManager) m_cosmeticManager->setCosmeticFilteringEnabled(enabled);
            });

    // 4. Create primary startup tab (loads YouTube)
    BrowserView* firstView = createBrowserTab(UrlPolicy::defaultUrl(), true);

    // 5. Connect initial load finish to splash screen
    connect(firstView, &QWebEngineView::loadFinished,
            this, &MainWindow::handleInitialLoadFinished);

    // 6. Splash screen overlay
    m_splashScreen = new SplashScreen(this);
    m_splashScreen->setGeometry(rect());
    m_splashScreen->show();
    m_splashScreen->raise();

    QTimer::singleShot(2500, this, [this]() {
        if (m_splashScreen) {
            m_splashScreen->finishWithFade();
            m_splashScreen = nullptr;
        }
    });
}

MainWindow::~MainWindow() = default;

// ============================================================
//  Tab Management
// ============================================================

BrowserView* MainWindow::createBrowserTab(const QUrl& url, bool setAsCurrent)
{
    auto* view = new BrowserView(this);

    // First tab initializes the shared persistent profile & cosmetic script
    if (!m_sharedProfile) {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);

        view->setProfileStoragePath(appDataDir + "/profile");
        view->applyCustomUserAgent();
        view->page()->profile()->setHttpAcceptLanguage("en-US,en;q=0.9");
        view->page()->profile()->setUrlRequestInterceptor(m_filterManager->interceptor());

        m_sharedProfile = view->page()->profile();

        m_cosmeticManager = std::make_unique<CosmeticFilterManager>(m_sharedProfile, this);
        m_cosmeticManager->setCosmeticFilteringEnabled(
            m_settingsManager->isCosmeticFilteringEnabled());
    }

    auto* nav = new NavigationManager(view, this);
    m_navManagers[view] = nav;

    // Connect link factory for new tabs (Ctrl+Click / window.open)
    view->setNewTabFactory([this]() -> BrowserView* {
        return createBrowserTab(QUrl(), true);
    });

    int idx = m_tabBar->addTab(QIcon(":/icons/app_icon.png"), "New Tab");
    m_stackedWidget->addWidget(view);

    if (setAsCurrent) {
        m_tabBar->setCurrentIndex(idx);
        m_stackedWidget->setCurrentIndex(idx);
        connectTabSignals(view, nav);
    }

    // Update tab title and favicon
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString& title) {
        updateTabTitle(view, title);
    });

    connect(view->page(), &QWebEnginePage::iconChanged, this, [this, view](const QIcon& icon) {
        int i = m_stackedWidget->indexOf(view);
        if (i >= 0 && !icon.isNull()) {
            m_tabBar->setTabIcon(i, icon);
        }
    });

    if (url.isValid() && !url.isEmpty()) {
        nav->load(url);
    }

    return view;
}

void MainWindow::openNewTab(const QUrl& url)
{
    QUrl target = (url.isValid() && !url.isEmpty()) ? url : UrlPolicy::defaultUrl();
    createBrowserTab(target, true);
}

BrowserView* MainWindow::currentBrowserView() const
{
    return qobject_cast<BrowserView*>(m_stackedWidget->currentWidget());
}

NavigationManager* MainWindow::currentNavManager() const
{
    BrowserView* v = currentBrowserView();
    return v ? m_navManagers.value(v, nullptr) : nullptr;
}

void MainWindow::connectTabSignals(BrowserView* view, NavigationManager* nav)
{
    disconnectTabSignals();

    m_connNavChanged   = connect(nav,  &NavigationManager::navigationChanged,
                                 this, &MainWindow::handleNavigationChanged);
    m_connLoadProgress = connect(nav,  &NavigationManager::loadingProgress,
                                 this, &MainWindow::handleLoadingProgress);
    m_connFullScreen   = connect(view, &BrowserView::fullScreenToggled,
                                 this, &MainWindow::handleFullScreenToggled);

    handleNavigationChanged(view->url(), view->title());
}

void MainWindow::disconnectTabSignals()
{
    disconnect(m_connNavChanged);
    disconnect(m_connLoadProgress);
    disconnect(m_connFullScreen);
}

void MainWindow::updateTabTitle(BrowserView* view, const QString& title)
{
    int i = m_stackedWidget->indexOf(view);
    if (i < 0) return;

    QString label = title.trimmed().isEmpty() ? "New Tab" : title.trimmed();
    if (label.length() > 28) {
        label = label.left(25) + "...";
    }
    m_tabBar->setTabText(i, label);

    if (view == currentBrowserView()) {
        setWindowTitle(title.isEmpty() ? "Yotobe" : QString("%1 - Yotobe").arg(title));
    }
}

void MainWindow::handleTabChanged(int index)
{
    if (index >= 0 && index < m_stackedWidget->count()) {
        m_stackedWidget->setCurrentIndex(index);
        BrowserView* view = currentBrowserView();
        NavigationManager* nav = currentNavManager();
        if (view && nav) {
            connectTabSignals(view, nav);
        }
    }
}

void MainWindow::handleTabCloseRequested(int index)
{
    if (m_tabBar->count() <= 1) return; // Keep at least one tab open

    auto* view = qobject_cast<BrowserView*>(m_stackedWidget->widget(index));
    m_tabBar->removeTab(index);
    if (view) {
        m_stackedWidget->removeWidget(view);
        NavigationManager* nav = m_navManagers.take(view);
        delete nav;
        view->stop();
        view->deleteLater();
    }
}

// ============================================================
//  Events
// ============================================================

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (m_splashScreen) {
        m_splashScreen->setGeometry(rect());
        m_splashScreen->raise();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        handleFullScreenToggled(false);
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::handleInitialLoadFinished(bool /*success*/)
{
    if (!m_firstPageLoaded) {
        m_firstPageLoaded = true;
        if (m_splashScreen) {
            m_splashScreen->finishWithFade();
            m_splashScreen = nullptr;
        }
    }
}

// ============================================================
//  UI Construction
// ============================================================

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // ============================================================
    // Row 1: Native Top Tab Strip (tabs on top, like Chrome/Edge)
    // ============================================================
    m_tabBarContainer = new QWidget(this);
    m_tabBarContainer->setObjectName("tabBarContainer");
    m_tabBarContainer->setFixedHeight(36);

    auto* tabContainerLayout = new QHBoxLayout(m_tabBarContainer);
    tabContainerLayout->setContentsMargins(6, 4, 6, 0);
    tabContainerLayout->setSpacing(4);

    m_tabBar = new QTabBar(m_tabBarContainer);
    m_tabBar->setObjectName("windowTabBar");
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setIconSize(QSize(16, 16));

    connect(m_tabBar, &QTabBar::currentChanged,    this, &MainWindow::handleTabChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::handleTabCloseRequested);
    tabContainerLayout->addWidget(m_tabBar);

    m_newTabBtn = new QToolButton(m_tabBarContainer);
    m_newTabBtn->setObjectName("newTabBtn");
    m_newTabBtn->setText("+");
    m_newTabBtn->setToolTip("New Tab (Ctrl+T)");
    m_newTabBtn->setFixedSize(28, 28);
    connect(m_newTabBtn, &QToolButton::clicked, [this]() { openNewTab(); });
    tabContainerLayout->addWidget(m_newTabBtn);

    tabContainerLayout->addStretch(1);

    centralLayout->addWidget(m_tabBarContainer);

    // ============================================================
    // Row 2: Unified Navigation Toolbar
    // ============================================================
    m_topBar = new QWidget(this);
    m_topBar->setObjectName("topBar");
    m_topBar->setFixedHeight(44);

    auto* topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(10, 4, 10, 4);
    topLayout->setSpacing(6);

    // Brand logo + text
    auto* brandLayout = new QHBoxLayout();
    brandLayout->setSpacing(6);

    m_brandLogo = new QLabel(m_topBar);
    QPixmap logoPixmap(":/icons/app_icon.png");
    if (!logoPixmap.isNull()) {
        QPixmap scaled = logoPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmap rounded(24, 24);
        rounded.fill(Qt::transparent);
        QPainter p(&rounded);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(0, 0, 24, 24, 5, 5);
        p.setClipPath(path);
        p.drawPixmap(0, 0, scaled);
        p.end();
        m_brandLogo->setPixmap(rounded);
    }
    brandLayout->addWidget(m_brandLogo);

    m_brandText = new QLabel(
        "<span style='font-size:15px;font-weight:800;color:#ffffff;'>Yotobe</span>"
        "<span style='color:#ff0000;font-size:15px;font-weight:800;'>.</span>",
        m_topBar);
    brandLayout->addWidget(m_brandText);
    topLayout->addLayout(brandLayout);
    topLayout->addSpacing(4);

    // Navigation buttons (All uniformly 32x32)
    m_backBtn = new QToolButton(m_topBar);
    m_backBtn->setIcon(QIcon(":/icons/back.svg"));
    m_backBtn->setIconSize(QSize(16, 16));
    m_backBtn->setToolTip("Back (Alt+Left)");
    m_backBtn->setFixedSize(32, 32);
    connect(m_backBtn, &QToolButton::clicked, [this]() {
        if (auto* n = currentNavManager()) n->goBack();
    });
    topLayout->addWidget(m_backBtn);

    m_forwardBtn = new QToolButton(m_topBar);
    m_forwardBtn->setIcon(QIcon(":/icons/forward.svg"));
    m_forwardBtn->setIconSize(QSize(16, 16));
    m_forwardBtn->setToolTip("Forward (Alt+Right)");
    m_forwardBtn->setFixedSize(32, 32);
    connect(m_forwardBtn, &QToolButton::clicked, [this]() {
        if (auto* n = currentNavManager()) n->goForward();
    });
    topLayout->addWidget(m_forwardBtn);

    m_reloadBtn = new QToolButton(m_topBar);
    m_reloadBtn->setIcon(QIcon(":/icons/reload.svg"));
    m_reloadBtn->setIconSize(QSize(16, 16));
    m_reloadBtn->setToolTip("Reload (F5)");
    m_reloadBtn->setFixedSize(32, 32);
    connect(m_reloadBtn, &QToolButton::clicked, [this]() {
        if (auto* n = currentNavManager()) n->reload();
    });
    topLayout->addWidget(m_reloadBtn);

    m_homeBtn = new QToolButton(m_topBar);
    m_homeBtn->setIcon(QIcon(":/icons/home.svg"));
    m_homeBtn->setIconSize(QSize(16, 16));
    m_homeBtn->setToolTip("YouTube Home");
    m_homeBtn->setFixedSize(32, 32);
    connect(m_homeBtn, &QToolButton::clicked, [this]() {
        if (auto* n = currentNavManager()) n->goHome();
    });
    topLayout->addWidget(m_homeBtn);

    topLayout->addSpacing(4);

    // Address bar (Height 32, stretches smoothly)
    m_addressEdit = new QLineEdit(m_topBar);
    m_addressEdit->setObjectName("addressEdit");
    m_addressEdit->setPlaceholderText("Search YouTube or enter URL...  (Ctrl+L)");
    m_addressEdit->setClearButtonEnabled(true);
    m_addressEdit->setFixedHeight(32);
    m_addressEdit->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    connect(m_addressEdit, &QLineEdit::returnPressed, this, &MainWindow::handleAddressEntered);
    topLayout->addWidget(m_addressEdit, 1);

    topLayout->addSpacing(4);

    // Action buttons (All 32px height)
    m_shieldBtn = new QToolButton(m_topBar);
    m_shieldBtn->setObjectName("shieldBtn");
    m_shieldBtn->setIcon(QIcon(":/icons/shield.svg"));
    m_shieldBtn->setIconSize(QSize(15, 15));
    m_shieldBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_shieldBtn->setText("0 Blocked");
    m_shieldBtn->setToolTip("Ad-Filter Status (click for details)");
    m_shieldBtn->setFixedHeight(32);
    connect(m_shieldBtn, &QToolButton::clicked, this, &MainWindow::openSettingsDialog);
    topLayout->addWidget(m_shieldBtn);

    m_downloadBtn = new QPushButton("Download", m_topBar);
    m_downloadBtn->setObjectName("downloadBtn");
    m_downloadBtn->setIcon(QIcon(":/icons/download.svg"));
    m_downloadBtn->setIconSize(QSize(15, 15));
    m_downloadBtn->setToolTip("Download current video (yt-dlp)");
    m_downloadBtn->setFixedHeight(32);
    connect(m_downloadBtn, &QPushButton::clicked, this, &MainWindow::openDownloadDialog);
    topLayout->addWidget(m_downloadBtn);

    m_settingsBtn = new QToolButton(m_topBar);
    m_settingsBtn->setIcon(QIcon(":/icons/settings.svg"));
    m_settingsBtn->setIconSize(QSize(16, 16));
    m_settingsBtn->setToolTip("Settings");
    m_settingsBtn->setFixedSize(32, 32);
    connect(m_settingsBtn, &QToolButton::clicked, this, &MainWindow::openSettingsDialog);
    topLayout->addWidget(m_settingsBtn);

    m_aboutBtn = new QToolButton(m_topBar);
    m_aboutBtn->setObjectName("aboutBtn");
    m_aboutBtn->setText("About");
    m_aboutBtn->setToolTip("Credits and Information");
    m_aboutBtn->setFixedHeight(32);
    connect(m_aboutBtn, &QToolButton::clicked, this, &MainWindow::openAboutDialog);
    topLayout->addWidget(m_aboutBtn);

    centralLayout->addWidget(m_topBar);

    // ============================================================
    // Row 3: Loading Progress Bar
    // ============================================================
    m_loadingBar = new QProgressBar(this);
    m_loadingBar->setFixedHeight(2);
    m_loadingBar->setTextVisible(false);
    m_loadingBar->setRange(0, 100);
    m_loadingBar->setValue(0);
    m_loadingBar->setStyleSheet(
        "QProgressBar { border:none; background:transparent; }"
        "QProgressBar::chunk { background-color:#ff0000; }");
    centralLayout->addWidget(m_loadingBar);

    // ============================================================
    // Row 4: Web Content Stack
    // ============================================================
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setObjectName("contentStack");
    centralLayout->addWidget(m_stackedWidget, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupShortcuts()
{
    // Ctrl+L — Focus address bar
    auto* focusAddress = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
    connect(focusAddress, &QShortcut::activated, [this]() {
        m_addressEdit->setFocus();
        m_addressEdit->selectAll();
    });

    // Ctrl+T — New Tab
    auto* newTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(newTab, &QShortcut::activated, [this]() { openNewTab(); });

    // Ctrl+W — Close Tab
    auto* closeTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(closeTab, &QShortcut::activated, [this]() {
        handleTabCloseRequested(m_tabBar->currentIndex());
    });

    // Ctrl+Tab — Next Tab
    auto* nextTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
    connect(nextTab, &QShortcut::activated, [this]() {
        if (m_tabBar->count() > 0) {
            int next = (m_tabBar->currentIndex() + 1) % m_tabBar->count();
            m_tabBar->setCurrentIndex(next);
        }
    });

    // Ctrl+Shift+Tab — Previous Tab
    auto* prevTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
    connect(prevTab, &QShortcut::activated, [this]() {
        if (m_tabBar->count() > 0) {
            int prev = (m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count();
            m_tabBar->setCurrentIndex(prev);
        }
    });

    // Ctrl+1 through Ctrl+9 — Direct tab jump
    for (int i = 1; i <= 9; ++i) {
        auto* sc = new QShortcut(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + i)), this);
        connect(sc, &QShortcut::activated, [this, i]() {
            int idx = i - 1;
            if (idx < m_tabBar->count()) m_tabBar->setCurrentIndex(idx);
        });
    }

    // Alt+Left — Back
    auto* back = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Left), this);
    connect(back, &QShortcut::activated, [this]() {
        if (auto* n = currentNavManager()) n->goBack();
    });

    // Alt+Right — Forward
    auto* forward = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Right), this);
    connect(forward, &QShortcut::activated, [this]() {
        if (auto* n = currentNavManager()) n->goForward();
    });

    // F5 / Ctrl+R — Reload
    auto* reloadF5 = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(reloadF5, &QShortcut::activated, [this]() {
        if (auto* n = currentNavManager()) n->reload();
    });
    auto* reloadCtrlR = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this);
    connect(reloadCtrlR, &QShortcut::activated, [this]() {
        if (auto* n = currentNavManager()) n->reload();
    });

    // F11 — Fullscreen toggle
    auto* f11 = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(f11, &QShortcut::activated, [this]() {
        handleFullScreenToggled(!isFullScreen());
    });
}

// ============================================================
//  Toolbar & State Sync
// ============================================================

void MainWindow::handleAddressEntered()
{
    QString input = m_addressEdit->text().trimmed();
    if (!input.isEmpty()) {
        if (auto* n = currentNavManager()) {
            n->navigateToInput(input);
        }
    }
}

void MainWindow::handleNavigationChanged(const QUrl& url, const QString& title)
{
    if (!m_addressEdit->hasFocus()) {
        m_addressEdit->setText(url.toString());
    }

    if (!title.isEmpty()) {
        setWindowTitle(QString("%1 - Yotobe").arg(title));
    } else {
        setWindowTitle("Yotobe");
    }

    if (auto* nav = currentNavManager()) {
        m_backBtn->setEnabled(nav->canGoBack());
        m_forwardBtn->setEnabled(nav->canGoForward());
    }
}

void MainWindow::handleLoadingProgress(int progress)
{
    m_loadingBar->setValue(progress);
    m_loadingBar->setVisible(progress > 0 && progress < 100);
}

void MainWindow::handleFullScreenToggled(bool fullScreen)
{
    if (fullScreen) {
        m_wasMaximizedBeforeFullscreen = isMaximized();
        m_tabBarContainer->hide();
        m_topBar->hide();
        m_loadingBar->hide();
        showFullScreen();
    } else {
        m_tabBarContainer->show();
        m_topBar->show();
        if (m_wasMaximizedBeforeFullscreen) {
            showMaximized();
        } else {
            showNormal();
        }
    }
}

void MainWindow::updateFilterPill()
{
    if (!m_filterManager || !m_filterManager->statistics()) return;

    if (!m_filterManager->isFilteringEnabled()) {
        m_shieldBtn->setText("Filter: Off");
        m_shieldBtn->setStyleSheet(
            "QToolButton#shieldBtn { background-color:#272727; color:#888888; border:1px solid #383838;"
            " border-radius:16px; padding:3px 12px; font-weight:600; font-size:12px; }"
            "QToolButton#shieldBtn:hover { background-color:#333333; }");
        return;
    }

    qint64 blocked = m_filterManager->statistics()->blockedRequests();
    m_shieldBtn->setText(QString("%1 Blocked").arg(blocked));
    m_shieldBtn->setStyleSheet(
        "QToolButton#shieldBtn { background-color:#122818; color:#4ade80; border:1px solid #1c5427;"
        " border-radius:16px; padding:3px 12px; font-weight:700; font-size:12px; }"
        "QToolButton#shieldBtn:hover { background-color:#163620; border-color:#267335; }");
}

// ============================================================
//  Dialogs
// ============================================================

void MainWindow::openSettingsDialog()
{
    if (!m_sharedProfile) return;
    SettingsDialog dialog(m_settingsManager.get(), m_filterManager.get(), m_sharedProfile, this);
    dialog.exec();
}

void MainWindow::openDownloadDialog()
{
    QUrl currentUrl = currentBrowserView() ? currentBrowserView()->url() : QUrl();
    DownloadDialog dialog(m_videoDownloader.get(), currentUrl, this);
    dialog.exec();
}

void MainWindow::openAboutDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("About Yotobe");
    dialog.setFixedSize(460, 340);
    dialog.setStyleSheet("background-color:#141414; color:#f1f1f1;");

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 28, 28, 24);
    layout->setSpacing(10);

    auto* logoLabel = new QLabel(&dialog);
    QPixmap raw(":/icons/app_icon.png");
    if (!raw.isNull()) {
        logoLabel->setPixmap(raw.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    auto* titleLabel = new QLabel(
        "<span style='font-size:22px;font-weight:800;color:#ffffff;'>Yotobe</span> "
        "<span style='font-size:13px;color:#ff4444;'>v1.0.0</span>",
        &dialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto* authorLabel = new QLabel("Made by Muhammad Haris Zubair", &dialog);
    authorLabel->setStyleSheet("color:#e2b714; font-size:14px; font-weight:700; letter-spacing:0.5px;");
    authorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(authorLabel);

    auto* descLabel = new QLabel(
        "High-performance desktop client for YouTube with native network ad-filtering, "
        "multi-tab browsing, cosmetic cleaning, and integrated video downloader.",
        &dialog);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color:#999999; font-size:12px;");
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);

    layout->addStretch();

    auto* okBtn = new QPushButton("Close", &dialog);
    okBtn->setStyleSheet(
        "background-color:#272727; color:#f1f1f1; border:1px solid #383838;"
        " border-radius:6px; padding:6px 20px; font-weight:600;");
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(okBtn, 0, Qt::AlignCenter);

    dialog.exec();
}

// ============================================================
//  Theme & Styles
// ============================================================

void MainWindow::applyTheme()
{
    AppTheme theme = m_settingsManager->theme();
    if (theme == AppTheme::Dark || theme == AppTheme::System) {
        setStyleSheet(
            // Window Base
            "QMainWindow { background-color:#0f0f0f; }"
            "QWidget#centralWidget { background-color:#0f0f0f; font-family:'Segoe UI Variable Display','Segoe UI',sans-serif; }"
            "QWidget#contentStack { background-color:#0f0f0f; border:none; }"

            // Tab Bar Strip (Row 1)
            "QWidget#tabBarContainer { background-color:#121212; border-bottom:1px solid #1e1e1e; }"
            "QTabBar#windowTabBar { background:transparent; }"
            "QTabBar#windowTabBar::tab {"
            " background-color:#181818; color:#888888;"
            " border:1px solid #242424; border-bottom:none;"
            " border-top-left-radius:8px; border-top-right-radius:8px;"
            " padding:5px 28px 5px 12px;"
            " min-width:110px; max-width:200px; height:20px;"
            " font-size:12px; font-weight:500; margin-right:3px;"
            "}"
            "QTabBar#windowTabBar::tab:selected {"
            " background-color:#212121; color:#ffffff;"
            " border-color:#2e2e2e; border-top:2px solid #ff0000; padding-top:4px;"
            "}"
            "QTabBar#windowTabBar::tab:hover:!selected {"
            " background-color:#1d1d1d; color:#dddddd;"
            "}"
            "QTabBar#windowTabBar::close-button {"
            " image:url(:/icons/close.svg);"
            " subcontrol-origin:padding; subcontrol-position:right;"
            " width:10px; height:10px; padding:3px; border-radius:8px; margin-right:3px;"
            "}"
            "QTabBar#windowTabBar::close-button:hover {"
            " background-color:#383838;"
            "}"
            "QToolButton#newTabBtn {"
            " background:transparent; color:#888888; border:none;"
            " font-size:18px; font-weight:300; border-radius:14px;"
            "}"
            "QToolButton#newTabBtn:hover {"
            " background-color:#2a2a2a; color:#ffffff;"
            "}"

            // Navigation Toolbar (Row 2)
            "QWidget#topBar { background-color:#161616; border-bottom:1px solid #222222; }"
            "QLineEdit#addressEdit {"
            " background-color:#1f1f1f; color:#ffffff; border:1px solid #303030;"
            " border-radius:16px; padding:2px 14px; font-size:13px;"
            " selection-background-color:#3ea6ff;"
            "}"
            "QLineEdit#addressEdit:focus { border:1px solid #3ea6ff; background-color:#242424; }"

            // Uniform ToolButtons in Toolbar
            "QToolButton {"
            " background-color:#212121; color:#f1f1f1; border:1px solid #333333;"
            " border-radius:16px; padding:2px; font-size:12px;"
            "}"
            "QToolButton:hover { background-color:#333333; border-color:#4a4a4a; }"
            "QToolButton:pressed { background-color:#3f3f3f; }"
            "QToolButton:disabled { background-color:#181818; color:#444444; border-color:#242424; }"

            // Download Button
            "QPushButton#downloadBtn {"
            " background-color:#cc0000; color:#ffffff; border:none;"
            " border-radius:16px; padding:4px 14px; font-weight:700; font-size:12px;"
            "}"
            "QPushButton#downloadBtn:hover { background-color:#e60000; }"
            "QPushButton#downloadBtn:pressed { background-color:#990000; }"

            // About Button
            "QToolButton#aboutBtn { padding:3px 12px; }"

            // Dialog Lists
            "QListWidget { background-color:#181818; color:#e0e0e0; border:1px solid #2e2e2e; border-radius:6px; padding:4px; }"
        );
    } else {
        setStyleSheet(
            "QMainWindow { background-color:#f9f9f9; }"
            "QWidget#centralWidget { background-color:#f9f9f9; font-family:'Segoe UI Variable Display','Segoe UI',sans-serif; }"
            "QWidget#tabBarContainer { background-color:#eaeaea; border-bottom:1px solid #d4d4d4; }"
            "QTabBar#windowTabBar::tab {"
            " background-color:#e0e0e0; color:#555555;"
            " border:1px solid #cccccc; border-bottom:none;"
            " border-top-left-radius:8px; border-top-right-radius:8px;"
            " padding:5px 28px 5px 12px; min-width:110px; max-width:200px; height:20px; font-size:12px; margin-right:3px;"
            "}"
            "QTabBar#windowTabBar::tab:selected {"
            " background-color:#f9f9f9; color:#000000;"
            " border-top:2px solid #cc0000; padding-top:4px;"
            "}"
            "QTabBar#windowTabBar::close-button {"
            " image:url(:/icons/close.svg);"
            " subcontrol-origin:padding; subcontrol-position:right;"
            " width:10px; height:10px; padding:3px; border-radius:8px; margin-right:3px;"
            "}"
            "QToolButton#newTabBtn { background:transparent; color:#555555; border:none; font-size:18px; border-radius:14px; }"
            "QToolButton#newTabBtn:hover { background-color:#dcdcdc; color:#000000; }"
            "QWidget#topBar { background-color:#f0f0f0; border-bottom:1px solid #e0e0e0; }"
            "QLineEdit#addressEdit { background-color:#ffffff; color:#000000; border:1px solid #cccccc; border-radius:16px; padding:2px 14px; font-size:13px; }"
            "QLineEdit#addressEdit:focus { border:1px solid #065fd4; }"
            "QToolButton { background-color:#f5f5f5; color:#0f0f0f; border:1px solid #d4d4d4; border-radius:16px; padding:2px; }"
            "QToolButton:hover { background-color:#e8e8e8; }"
            "QPushButton#downloadBtn { background-color:#cc0000; color:#ffffff; border-radius:16px; padding:4px 14px; font-weight:700; font-size:12px; }"
            "QPushButton#downloadBtn:hover { background-color:#e60000; }"
            "QToolButton#aboutBtn { padding:3px 12px; }"
        );
    }
    updateFilterPill();
}
