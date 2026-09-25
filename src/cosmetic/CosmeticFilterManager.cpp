#include "CosmeticFilterManager.h"
#include <QWebEngineProfile>
#include <QWebEngineScriptCollection>
#include <QFile>
#include <QTextStream>
#include <QDebug>

const QString CosmeticFilterManager::SCRIPT_NAME = "YotobeCosmeticFilterScript";

CosmeticFilterManager::CosmeticFilterManager(QWebEngineProfile* profile, QObject* parent)
    : QObject(parent)
    , m_profile(profile)
{
    if (m_enabled) {
        injectScript();
    }
}

void CosmeticFilterManager::setCosmeticFilteringEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (m_enabled) {
        injectScript();
    } else {
        removeScript();
    }
}

bool CosmeticFilterManager::isCosmeticFilteringEnabled() const {
    return m_enabled;
}

void CosmeticFilterManager::injectScript() {
    if (!m_profile) return;

    QFile file(":/scripts/cosmetic_filters.js");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open cosmetic filter script resource!";
        return;
    }

    QTextStream in(&file);
    QString scriptSource = in.readAll();
    file.close();

    QWebEngineScript script;
    script.setName(SCRIPT_NAME);
    script.setSourceCode(scriptSource);
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(false);

    // Remove existing if already present
    removeScript();

    m_profile->scripts()->insert(script);
}

void CosmeticFilterManager::removeScript() {
    if (!m_profile) return;
    QList<QWebEngineScript> scripts = m_profile->scripts()->find(SCRIPT_NAME);
    for (const auto& s : scripts) {
        m_profile->scripts()->remove(s);
    }
}
