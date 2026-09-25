#include "SettingsDialog.h"
#include "SettingsManager.h"
#include "FilterManager.h"
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>

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
    setMinimumSize(600, 480);
    setupUi();
    updateStatsDisplay();

    if (m_filterMgr && m_filterMgr->statistics()) {
        connect(m_filterMgr->statistics().get(), &FilterStatistics::statsChanged,
                this, &SettingsDialog::updateStatsDisplay);
    }
}

void SettingsDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    auto tabs = new QTabWidget(this);

    // Tab 1: General & Appearance
    auto generalWidget = new QWidget();
    auto generalLayout = new QFormLayout(generalWidget);

    m_themeCombo = new QComboBox();
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
    generalLayout->addRow("Application Theme:", m_themeCombo);

    auto hwAccelCheck = new QCheckBox("Enable Hardware Accelerated Rendering");
    if (m_settingsMgr) {
        hwAccelCheck->setChecked(m_settingsMgr->isHardwareAccelerationEnabled());
        connect(hwAccelCheck, &QCheckBox::toggled, [this](bool checked) {
            m_settingsMgr->setHardwareAccelerationEnabled(checked);
        });
    }
    generalLayout->addRow("Performance:", hwAccelCheck);
    tabs->addTab(generalWidget, "General");

    // Tab 2: Filtering & Statistics
    auto filterWidget = new QWidget();
    auto filterLayout = new QVBoxLayout(filterWidget);

    m_networkFilteringCheck = new QCheckBox("Enable Network-Level Filtering (Block Ad & Tracker Requests)");
    m_networkFilteringCheck->setChecked(m_settingsMgr ? m_settingsMgr->isFilteringEnabled() : true);
    connect(m_networkFilteringCheck, &QCheckBox::toggled, [this](bool checked) {
        if (m_settingsMgr) m_settingsMgr->setFilteringEnabled(checked);
        if (m_filterMgr) m_filterMgr->setFilteringEnabled(checked);
    });
    filterLayout->addWidget(m_networkFilteringCheck);

    m_cosmeticFilteringCheck = new QCheckBox("Enable Cosmetic DOM Filtering (Hide Promo Slots & Banner Elements)");
    m_cosmeticFilteringCheck->setChecked(m_settingsMgr ? m_settingsMgr->isCosmeticFilteringEnabled() : true);
    connect(m_cosmeticFilteringCheck, &QCheckBox::toggled, [this](bool checked) {
        if (m_settingsMgr) m_settingsMgr->setCosmeticFilteringEnabled(checked);
    });
    filterLayout->addWidget(m_cosmeticFilteringCheck);

    m_ruleCountLabel = new QLabel(QString("Active Filter Rules: %1").arg(m_filterMgr ? m_filterMgr->totalRulesLoaded() : 0));
    m_ruleCountLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");
    filterLayout->addWidget(m_ruleCountLabel);

    m_statsLabel = new QLabel("Requests: 0 total | 0 allowed | 0 blocked");
    filterLayout->addWidget(m_statsLabel);

    auto btnRow = new QHBoxLayout();
    auto resetStatsBtn = new QPushButton("Reset Statistics");
    connect(resetStatsBtn, &QPushButton::clicked, this, &SettingsDialog::resetStatsClicked);
    btnRow->addWidget(resetStatsBtn);

    auto addRulesBtn = new QPushButton("Import External Filter List (.txt)...");
    connect(addRulesBtn, &QPushButton::clicked, this, &SettingsDialog::addCustomFilterFileClicked);
    btnRow->addWidget(addRulesBtn);
    btnRow->addStretch();
    filterLayout->addLayout(btnRow);

    filterLayout->addWidget(new QLabel("Recent Blocked Requests Log:"));
    m_logList = new QListWidget();
    filterLayout->addWidget(m_logList);

    tabs->addTab(filterWidget, "Filtering");

    // Tab 3: Privacy & Data
    auto privacyWidget = new QWidget();
    auto privacyLayout = new QVBoxLayout(privacyWidget);

    privacyLayout->addWidget(new QLabel("Manage browsing data stored in your local Yotobe profile:"));

    auto clearCacheBtn = new QPushButton("Clear Web Cache");
    connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsDialog::clearCacheClicked);
    privacyLayout->addWidget(clearCacheBtn);

    auto clearCookiesBtn = new QPushButton("Clear Cookies & Local Storage (Logs Out Current Session)");
    connect(clearCookiesBtn, &QPushButton::clicked, this, &SettingsDialog::clearCookiesClicked);
    privacyLayout->addWidget(clearCookiesBtn);

    privacyLayout->addStretch();
    tabs->addTab(privacyWidget, "Privacy");

    mainLayout->addWidget(tabs);

    auto footerLayout = new QHBoxLayout();
    auto authorLabel = new QLabel("Made by Muhammad Haris Zubair", this);
    authorLabel->setStyleSheet("color: #e2b714; font-size: 12px; font-weight: 600;");
    footerLayout->addWidget(authorLabel);
    footerLayout->addStretch();

    auto closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet("background-color: #272727; color: #f1f1f1; border: 1px solid #383838; border-radius: 6px; padding: 6px 18px; font-weight: 600;");
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
        m_ruleCountLabel->setText(QString("Active Filter Rules: %1").arg(m_filterMgr->totalRulesLoaded()));
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
