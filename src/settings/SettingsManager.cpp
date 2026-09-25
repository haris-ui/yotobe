#include "SettingsManager.h"
#include <QStandardPaths>

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings("YotobeApp", "Yotobe")
{
}

bool SettingsManager::isFilteringEnabled() const {
    return m_settings.value("filtering/network_enabled", true).toBool();
}

void SettingsManager::setFilteringEnabled(bool enabled) {
    m_settings.setValue("filtering/network_enabled", enabled);
    emit filteringSettingChanged(enabled);
}

bool SettingsManager::isCosmeticFilteringEnabled() const {
    return m_settings.value("filtering/cosmetic_enabled", true).toBool();
}

void SettingsManager::setCosmeticFilteringEnabled(bool enabled) {
    m_settings.setValue("filtering/cosmetic_enabled", enabled);
    emit cosmeticFilteringSettingChanged(enabled);
}

AppTheme SettingsManager::theme() const {
    int val = m_settings.value("ui/theme", static_cast<int>(AppTheme::Dark)).toInt();
    return static_cast<AppTheme>(val);
}

void SettingsManager::setTheme(AppTheme theme) {
    m_settings.setValue("ui/theme", static_cast<int>(theme));
    emit themeChanged(theme);
}

bool SettingsManager::isHardwareAccelerationEnabled() const {
    return m_settings.value("performance/hardware_accel", true).toBool();
}

void SettingsManager::setHardwareAccelerationEnabled(bool enabled) {
    m_settings.setValue("performance/hardware_accel", enabled);
}

QString SettingsManager::defaultDownloadPath() const {
    QString defaultMovies = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    return m_settings.value("downloads/path", defaultMovies).toString();
}

void SettingsManager::setDefaultDownloadPath(const QString& path) {
    m_settings.setValue("downloads/path", path);
}
