#pragma once

#include <QDialog>

class SettingsManager;
class FilterManager;
class QWebEngineProfile;
class QLabel;
class QCheckBox;
class QComboBox;
class QListWidget;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(SettingsManager* settingsMgr,
                            FilterManager* filterMgr,
                            QWebEngineProfile* profile,
                            QWidget* parent = nullptr);

private slots:
    void updateStatsDisplay();
    void resetStatsClicked();
    void clearCacheClicked();
    void clearCookiesClicked();
    void addCustomFilterFileClicked();

private:
    void setupUi();

    SettingsManager* m_settingsMgr{nullptr};
    FilterManager* m_filterMgr{nullptr};
    QWebEngineProfile* m_profile{nullptr};

    QCheckBox* m_networkFilteringCheck{nullptr};
    QCheckBox* m_cosmeticFilteringCheck{nullptr};
    QComboBox* m_themeCombo{nullptr};
    QLabel* m_statsLabel{nullptr};
    QLabel* m_ruleCountLabel{nullptr};
    QListWidget* m_logList{nullptr};
};
