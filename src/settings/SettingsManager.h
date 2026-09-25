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
    // Centralised key constants — a typo here is a compile error, not a silent registry split.
    static constexpr QLatin1StringView kKeyNetworkFilter  {"filtering/network_enabled"};
    static constexpr QLatin1StringView kKeyCosmeticFilter {"filtering/cosmetic_enabled"};
    static constexpr QLatin1StringView kKeyTheme          {"ui/theme"};
    static constexpr QLatin1StringView kKeyHwAccel        {"performance/hardware_accel"};
    static constexpr QLatin1StringView kKeyDownloadPath   {"downloads/path"};

    QSettings m_settings;
};
