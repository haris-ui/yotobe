#pragma once

#include <QObject>
#include <QSettings>

enum class AppTheme {
    Dark,
    Light,
    System
};

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject* parent = nullptr);

    bool isFilteringEnabled() const;
    void setFilteringEnabled(bool enabled);

    bool isCosmeticFilteringEnabled() const;
    void setCosmeticFilteringEnabled(bool enabled);

    AppTheme theme() const;
    void setTheme(AppTheme theme);

    bool isHardwareAccelerationEnabled() const;
    void setHardwareAccelerationEnabled(bool enabled);

    QString defaultDownloadPath() const;
    void setDefaultDownloadPath(const QString& path);

signals:
    void themeChanged(AppTheme theme);
    void filteringSettingChanged(bool enabled);
    void cosmeticFilteringSettingChanged(bool enabled);

private:
    QSettings m_settings;
};
