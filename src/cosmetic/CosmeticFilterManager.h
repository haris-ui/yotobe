#pragma once

#include <QObject>
#include <QWebEngineScript>

class QWebEngineProfile;

class CosmeticFilterManager : public QObject {
    Q_OBJECT
public:
    explicit CosmeticFilterManager(QWebEngineProfile* profile, QObject* parent = nullptr);

    void setCosmeticFilteringEnabled(bool enabled);
    bool isCosmeticFilteringEnabled() const;

private:
    void injectScript();
    void removeScript();

    QWebEngineProfile* m_profile{nullptr};
    bool m_enabled{true};
    static const QString SCRIPT_NAME;
};
