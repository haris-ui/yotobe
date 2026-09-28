#include "SettingsDialog.h"
#include "SettingsManager.h"
#include "FilterManager.h"
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

SettingsDialog::SettingsDialog(SettingsManager* settingsMgr,
                               FilterManager* filterMgr,
                               QWebEngineProfile* profile,
                               QWidget* parent)
    : QDialog(parent)
    , m_settingsMgr(settingsMgr)
    , m_filterMgr(filterMgr)
    , m_profile(profile)
{
    setWindowTitle("Yotobe - Settings");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setMinimumSize(640, 520);
    resize(660, 540);

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    COLORREF captionColor = RGB(9, 9, 9);
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    COLORREF borderColor = RGB(38, 38, 38);
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
#endif

    setStyleSheet(
        "QDialog {"
        "  background-color: #090909;"
        "  color: #ffffff;"
        "  font-family: 'GT Walsheim', 'Segoe UI Variable Text', 'Segoe UI', -apple-system, sans-serif;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #262626;"
        "  border-radius: 16px;"
        "  background-color: #141414;"
        "  top: -1px;"
        "}"
        "QTabBar {"
        "  qproperty-drawBase: 0;"
        "  background: transparent;"
        "}"
        "QTabBar::tab {"
        "  background: #090909;"
        "  color: #888888;"
        "  font-size: 13px;"
        "  font-weight: 500;"
        "  border: 1px solid #1a1a1a;"
        "  border-radius: 100px;"
        "  padding: 7px 18px;"
        "  margin-right: 6px;"
        "  margin-bottom: 8px;"
        "}"
        "QTabBar::tab:hover {"
        "  background: #141414;"
        "  color: #ffffff;"
        "  border-color: #262626;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1c1c1c;"
        "  color: #ffffff;"
        "  border: 1px solid #262626;"
        "}"
        "QLabel {"
        "  color: #999999;"
        "  font-size: 13px;"
        "}"
        "QCheckBox {"
        "  color: #ffffff;"
        "  font-size: 13px;"
        "  font-weight: 500;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 5px;"
        "  border: 1px solid #262626;"
        "  background-color: #1c1c1c;"
        "}"
        "QCheckBox::indicator:hover {"
        "  border-color: #383838;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #0099ff;"
        "  border-color: #0099ff;"
        "  image: url(:/icons/check.svg);"
        "}"
        "QComboBox {"
        "  background-color: #1c1c1c;"
        "  color: #ffffff;"
        "  border: 1px solid #262626;"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "  font-size: 13px;"
        "}"
        "QComboBox:focus {"
        "  border: 1px solid #0099ff;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 22px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/icons/chevron_down.svg);"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #1c1c1c;"
        "  color: #ffffff;"
        "  selection-background-color: #262626;"
        "  selection-color: #ffffff;"
        "  border: 1px solid #262626;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
        "QListWidget {"
        "  background-color: #0c0c0c;"
        "  color: #cccccc;"
        "  border: 1px solid #222222;"
        "  border-radius: 10px;"
        "  padding: 6px;"
        "  font-family: 'Consolas', 'Cascadia Code', monospace;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item {"
        "  padding: 4px 6px;"
        "  border-bottom: 1px solid #161616;"
        "}"
        "QScrollBar:vertical {"
        "  background: #0c0c0c;"
        "  width: 6px;"
        "  margin: 0;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #262626;"
        "  min-height: 20px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #383838;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
    );

    setupUi();
    updateStatsDisplay();

    if (m_filterMgr && m_filterMgr->statistics()) {
        connect(m_filterMgr->statistics().get(), &FilterStatistics::statsChanged,
                this, &SettingsDialog::updateStatsDisplay);
    }
}

void SettingsDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 18);
    mainLayout->setSpacing(14);

    auto tabs = new QTabWidget(this);

    const QString secondaryBtnStyle =
        "QPushButton {"
        "  background-color: #1c1c1c;"
        "  color: #ffffff;"
        "  font-weight: 500;"
        "  font-size: 12px;"
        "  border: 1px solid #262626;"
        "  border-radius: 100px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #242424;"
        "  border-color: #333333;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #181818;"
        "}";

    // ─────────────────────────────────────────────────────────────
    // Tab 1: General & Appearance
    // ─────────────────────────────────────────────────────────────
    auto generalWidget = new QWidget();
    auto generalLayout = new QVBoxLayout(generalWidget);
    generalLayout->setContentsMargins(22, 22, 22, 22);
    generalLayout->setSpacing(18);

    // Appearance Section
    auto appearanceHeader = new QLabel("APPEARANCE", generalWidget);
    appearanceHeader->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    generalLayout->addWidget(appearanceHeader);

    auto themeRow = new QHBoxLayout();
    themeRow->setSpacing(12);
    auto themeLabel = new QLabel("Theme Mode", generalWidget);
    themeLabel->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: 500;");
    themeRow->addWidget(themeLabel);

    m_themeCombo = new QComboBox(generalWidget);
    m_themeCombo->setFixedWidth(200);
    m_themeCombo->addItem("Dark Mode", static_cast<int>(AppTheme::Dark));
    m_themeCombo->addItem("Light Mode", static_cast<int>(AppTheme::Light));
    m_themeCombo->addItem("System Default", static_cast<int>(AppTheme::System));
    if (m_settingsMgr) {
        int idx = m_themeCombo->findData(static_cast<int>(m_settingsMgr->theme()));
        if (idx != -1) m_themeCombo->setCurrentIndex(idx);
        connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            AppTheme t = static_cast<AppTheme>(m_themeCombo->itemData(index).toInt());
            m_settingsMgr->setTheme(t);
        });
    }
    themeRow->addWidget(m_themeCombo);
    themeRow->addStretch();
    generalLayout->addLayout(themeRow);

    auto themeDesc = new QLabel("Controls the application's visual chrome and contrast palette.", generalWidget);
    themeDesc->setStyleSheet("color: #777777; font-size: 12px; margin-top: -6px;");
    generalLayout->addWidget(themeDesc);

    // Separator line
    auto sep1 = new QFrame(generalWidget);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #202020;");
    generalLayout->addWidget(sep1);

    // Performance Section
    auto perfHeader = new QLabel("PERFORMANCE & ACCELERATION", generalWidget);
    perfHeader->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    generalLayout->addWidget(perfHeader);

    auto hwAccelCheck = new QCheckBox("Enable Hardware Accelerated Rendering", generalWidget);
    if (m_settingsMgr) {
        hwAccelCheck->setChecked(m_settingsMgr->isHardwareAccelerationEnabled());
        connect(hwAccelCheck, &QCheckBox::toggled, [this](bool checked) {
            m_settingsMgr->setHardwareAccelerationEnabled(checked);
        });
    }
    generalLayout->addWidget(hwAccelCheck);

    auto hwAccelDesc = new QLabel("Utilizes the GPU for video decoding, WebGL, and compositing to lower CPU usage during 4K/60fps streaming.", generalWidget);
    hwAccelDesc->setWordWrap(true);
    hwAccelDesc->setStyleSheet("color: #777777; font-size: 12px; margin-top: -6px;");
    generalLayout->addWidget(hwAccelDesc);

    generalLayout->addStretch();
    tabs->addTab(generalWidget, "General");

    // ─────────────────────────────────────────────────────────────
    // Tab 2: Filtering & Statistics
    // ─────────────────────────────────────────────────────────────
    auto filterWidget = new QWidget();
    auto filterLayout = new QVBoxLayout(filterWidget);
    filterLayout->setContentsMargins(22, 20, 22, 20);
    filterLayout->setSpacing(12);

    auto filterHeader = new QLabel("CONTENT FILTERING", filterWidget);
    filterHeader->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    filterLayout->addWidget(filterHeader);

    m_networkFilteringCheck = new QCheckBox("Network-Level Filtering (Block Ad & Tracker Requests)", filterWidget);
    m_networkFilteringCheck->setChecked(m_settingsMgr ? m_settingsMgr->isFilteringEnabled() : true);
    connect(m_networkFilteringCheck, &QCheckBox::toggled, [this](bool checked) {
        if (m_settingsMgr) m_settingsMgr->setFilteringEnabled(checked);
        if (m_filterMgr) m_filterMgr->setFilteringEnabled(checked);
    });
    filterLayout->addWidget(m_networkFilteringCheck);

    m_cosmeticFilteringCheck = new QCheckBox("Cosmetic DOM Filtering (Hide Promo Slots & Banner Elements)", filterWidget);
    m_cosmeticFilteringCheck->setChecked(m_settingsMgr ? m_settingsMgr->isCosmeticFilteringEnabled() : true);
    connect(m_cosmeticFilteringCheck, &QCheckBox::toggled, [this](bool checked) {
        if (m_settingsMgr) m_settingsMgr->setCosmeticFilteringEnabled(checked);
    });
    filterLayout->addWidget(m_cosmeticFilteringCheck);

    // Stats sub-surface card
    auto statsCard = new QWidget(filterWidget);
    statsCard->setStyleSheet("background-color: #1a1a1a; border: 1px solid #262626; border-radius: 12px;");
    auto statsCardLayout = new QVBoxLayout(statsCard);
    statsCardLayout->setContentsMargins(14, 12, 14, 12);
    statsCardLayout->setSpacing(8);

    auto statsRow = new QHBoxLayout();
    m_ruleCountLabel = new QLabel(QString("Active Rules: %1").arg(m_filterMgr ? m_filterMgr->totalRulesLoaded() : 0), statsCard);
    m_ruleCountLabel->setStyleSheet("color: #22c55e; font-weight: 600; font-size: 12px; background: transparent; border: none;");
    statsRow->addWidget(m_ruleCountLabel);
    statsRow->addStretch();

    m_statsLabel = new QLabel("Requests: 0 total | 0 allowed | 0 blocked", statsCard);
    m_statsLabel->setStyleSheet("color: #999999; font-size: 12px; background: transparent; border: none;");
    statsRow->addWidget(m_statsLabel);
    statsCardLayout->addLayout(statsRow);

    auto btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    auto resetStatsBtn = new QPushButton("Reset Statistics", statsCard);
    resetStatsBtn->setStyleSheet(secondaryBtnStyle);
    connect(resetStatsBtn, &QPushButton::clicked, this, &SettingsDialog::resetStatsClicked);
    btnRow->addWidget(resetStatsBtn);

    auto addRulesBtn = new QPushButton("Import External Filter List (.txt)...", statsCard);
    addRulesBtn->setStyleSheet(secondaryBtnStyle);
    connect(addRulesBtn, &QPushButton::clicked, this, &SettingsDialog::addCustomFilterFileClicked);
    btnRow->addWidget(addRulesBtn);
    btnRow->addStretch();
    statsCardLayout->addLayout(btnRow);

    filterLayout->addWidget(statsCard);

    auto logHeader = new QLabel("Recent Blocked Requests Log:", filterWidget);
    logHeader->setStyleSheet("color: #888888; font-size: 12px; font-weight: 500; margin-top: 4px;");
    filterLayout->addWidget(logHeader);

    m_logList = new QListWidget(filterWidget);
    filterLayout->addWidget(m_logList, 1);

    tabs->addTab(filterWidget, "Filtering");

    // ─────────────────────────────────────────────────────────────
    // Tab 3: Privacy & Data
    // ─────────────────────────────────────────────────────────────
    auto privacyWidget = new QWidget();
    auto privacyLayout = new QVBoxLayout(privacyWidget);
    privacyLayout->setContentsMargins(22, 22, 22, 22);
    privacyLayout->setSpacing(16);

    auto privacyHeader = new QLabel("SESSION & BROWSING DATA", privacyWidget);
    privacyHeader->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    privacyLayout->addWidget(privacyHeader);

    // Card 1: Web Cache
    auto cacheCard = new QWidget(privacyWidget);
    cacheCard->setStyleSheet("background-color: #1a1a1a; border: 1px solid #262626; border-radius: 12px;");
    auto cacheLayout = new QHBoxLayout(cacheCard);
    cacheLayout->setContentsMargins(16, 14, 16, 14);
    cacheLayout->setSpacing(12);

    auto cacheInfo = new QVBoxLayout();
    cacheInfo->setSpacing(4);
    auto cacheTitle = new QLabel("Web Engine Disk Cache", cacheCard);
    cacheTitle->setStyleSheet("color: #ffffff; font-weight: 600; font-size: 13px; background: transparent; border: none;");
    cacheInfo->addWidget(cacheTitle);
    auto cacheDesc = new QLabel("Removes temporary video fragments, page scripts, and media thumbnails to reclaim disk space.", cacheCard);
    cacheDesc->setStyleSheet("color: #777777; font-size: 12px; background: transparent; border: none;");
    cacheDesc->setWordWrap(true);
    cacheInfo->addWidget(cacheDesc);
    cacheLayout->addLayout(cacheInfo, 1);

    auto clearCacheBtn = new QPushButton("Clear Cache", cacheCard);
    clearCacheBtn->setStyleSheet(secondaryBtnStyle);
    connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsDialog::clearCacheClicked);
    cacheLayout->addWidget(clearCacheBtn);
    privacyLayout->addWidget(cacheCard);

    // Card 2: Cookies & Session
    auto cookieCard = new QWidget(privacyWidget);
    cookieCard->setStyleSheet("background-color: #1a1a1a; border: 1px solid #262626; border-radius: 12px;");
    auto cookieLayout = new QHBoxLayout(cookieCard);
    cookieLayout->setContentsMargins(16, 14, 16, 14);
    cookieLayout->setSpacing(12);

    auto cookieInfo = new QVBoxLayout();
    cookieInfo->setSpacing(4);
    auto cookieTitle = new QLabel("Cookies & Stored Session", cookieCard);
    cookieTitle->setStyleSheet("color: #ffffff; font-weight: 600; font-size: 13px; background: transparent; border: none;");
    cookieInfo->addWidget(cookieTitle);
    auto cookieDesc = new QLabel("Deletes all persistent cookies and local storage tokens. Warning: This will log you out of YouTube.", cookieCard);
    cookieDesc->setStyleSheet("color: #777777; font-size: 12px; background: transparent; border: none;");
    cookieDesc->setWordWrap(true);
    cookieInfo->addWidget(cookieDesc);
    cookieLayout->addLayout(cookieInfo, 1);

    auto clearCookiesBtn = new QPushButton("Clear Cookies", cookieCard);
    clearCookiesBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #241414;"
        "  color: #ff6b6b;"
        "  font-weight: 500;"
        "  font-size: 12px;"
        "  border: 1px solid #3d2020;"
        "  border-radius: 100px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #331a1a;"
        "  border-color: #552828;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1c1010;"
        "}");
    connect(clearCookiesBtn, &QPushButton::clicked, this, &SettingsDialog::clearCookiesClicked);
    cookieLayout->addWidget(clearCookiesBtn);
    privacyLayout->addWidget(cookieCard);

    privacyLayout->addStretch();
    tabs->addTab(privacyWidget, "Privacy");

    mainLayout->addWidget(tabs);

    // ─────────────────────────────────────────────────────────────
    // Footer
    // ─────────────────────────────────────────────────────────────
    auto footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(4, 4, 4, 0);

    auto authorBadge = new QLabel(
        "<span style='color: #777777;'>Made by</span> <span style='color: #ffffff; font-weight: 600;'>Muhammad Haris Zubair</span>",
        this);
    authorBadge->setStyleSheet(
        "background-color: #141414;"
        "border: 1px solid #262626;"
        "border-radius: 100px;"
        "padding: 5px 14px;"
        "font-size: 11px;");
    footerLayout->addWidget(authorBadge);

    footerLayout->addStretch();

    // Primary White Pill Close Button
    auto closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #000000;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  border-radius: 100px;"
        "  padding: 8px 24px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e5e5e5;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #cccccc;"
        "}");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footerLayout->addWidget(closeBtn);

    mainLayout->addLayout(footerLayout);
}

void SettingsDialog::updateStatsDisplay() {
    if (!m_filterMgr || !m_filterMgr->statistics()) return;

    auto stats = m_filterMgr->statistics();
    m_statsLabel->setText(QString("Requests: %1 total | %2 allowed | %3 blocked")
                          .arg(stats->totalRequests())
                          .arg(stats->allowedRequests())
                          .arg(stats->blockedRequests()));

    if (m_ruleCountLabel) {
        m_ruleCountLabel->setText(QString("Active Rules: %1").arg(m_filterMgr->totalRulesLoaded()));
    }

    if (m_logList) {
        m_logList->clear();
        m_logList->addItems(stats->recentBlockedLogs(40));
    }
}

void SettingsDialog::resetStatsClicked() {
    if (m_filterMgr && m_filterMgr->statistics()) {
        m_filterMgr->statistics()->reset();
        updateStatsDisplay();
    }
}

void SettingsDialog::clearCacheClicked() {
    if (m_profile) {
        m_profile->clearHttpCache();
        QMessageBox::information(this, "Privacy", "Web cache successfully cleared.");
    }
}

void SettingsDialog::clearCookiesClicked() {
    if (m_profile) {
        m_profile->cookieStore()->deleteAllCookies();
        QMessageBox::information(this, "Privacy", "Cookies and session data cleared.");
    }
}

void SettingsDialog::addCustomFilterFileClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select Adblock Filter List", QString(), "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty() && m_filterMgr) {
        if (m_filterMgr->loadRulesFromFile(fileName)) {
            QMessageBox::information(this, "Filters Updated",
                                     QString("Successfully loaded rules from %1.\nTotal active rules: %2")
                                     .arg(fileName).arg(m_filterMgr->totalRulesLoaded()));
            updateStatsDisplay();
        } else {
            QMessageBox::warning(this, "Error", "Failed to parse filter rules from selected file.");
        }
    }
}
