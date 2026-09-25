#include "SettingsManager.h"
#include <QStandardPaths>

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings("YotobeApp", "Yotobe")
{
}

bool SettingsManager::isFilteringEnabled() const {
    return m_settings.value(kKeyNetworkFilter, true).toBool();
}

void SettingsManager::setFilteringEnabled(bool enabled) {
    m_settings.setValue(kKeyNetworkFilter, enabled);
    emit filteringSettingChanged(enabled);
}

bool SettingsManager::isCosmeticFilteringEnabled() const {
    return m_settings.value(kKeyCosmeticFilter, true).toBool();
}

void SettingsManager::setCosmeticFilteringEnabled(bool enabled) {
    m_settings.setValue(kKeyCosmeticFilter, enabled);
    emit cosmeticFilteringSettingChanged(enabled);
}

AppTheme SettingsManager::theme() const {
    int val = m_settings.value(kKeyTheme, static_cast<int>(AppTheme::Dark)).toInt();
    return static_cast<AppTheme>(val);
}

void SettingsManager::setTheme(AppTheme theme) {
    m_settings.setValue(kKeyTheme, static_cast<int>(theme));
    emit themeChanged(theme);
}

bool SettingsManager::isHardwareAccelerationEnabled() const {
    return m_settings.value(kKeyHwAccel, true).toBool();
}

void SettingsManager::setHardwareAccelerationEnabled(bool enabled) {
    m_settings.setValue(kKeyHwAccel, enabled);
}

QString SettingsManager::defaultDownloadPath() const {
    const QString fallback = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    return m_settings.value(kKeyDownloadPath, fallback).toString();
}

void SettingsManager::setDefaultDownloadPath(const QString& path) {
    m_settings.setValue(kKeyDownloadPath, path);
}
