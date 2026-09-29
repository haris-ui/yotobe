#include "CosmeticFilterManager.h"
#include <QWebEngineProfile>
#include <QWebEngineScriptCollection>
#include <QFile>
#include <QTextStream>
#include <QDebug>

const QString CosmeticFilterManager::SCRIPT_NAME = "YotobeCosmeticFilterScript";
const QString CosmeticFilterManager::STEALTH_SCRIPT_NAME = "YotobeBrowserCompatibilityScript";

CosmeticFilterManager::CosmeticFilterManager(QWebEngineProfile* profile, QObject* parent)
    : QObject(parent)
    , m_profile(profile)
{
    // Always inject browser compatibility polyfills (window.chrome, navigator.webdriver)
    injectCompatibilityScript();

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

void CosmeticFilterManager::injectCompatibilityScript() {
    if (!m_profile) return;

    // Remove existing if already registered
    QList<QWebEngineScript> existing = m_profile->scripts()->find(STEALTH_SCRIPT_NAME);
    for (const auto& s : existing) {
        m_profile->scripts()->remove(s);
    }

    const QString stealthCode = QStringLiteral(
        "(function() {"
        "  'use strict';"
        "  try {"
        "    if (!window.chrome) {"
        "      window.chrome = {"
        "        app: {"
        "          isInstalled: false,"
        "          InstallState: { DISABLED: 'disabled', INSTALLED: 'installed', NOT_INSTALLED: 'not_installed' },"
        "          RunningState: { CANNOT_RUN: 'cannot_run', READY_TO_RUN: 'ready_to_run', RUNNING: 'running' }"
        "        },"
        "        csi: function() {},"
        "        loadTimes: function() {"
        "          var t = (window.performance && window.performance.timeOrigin) ? window.performance.timeOrigin / 1000 : Date.now() / 1000;"
        "          return {"
        "            requestTime: t,"
        "            startLoadTime: t,"
        "            commitLoadTime: t,"
        "            finishDocumentLoadTime: t,"
        "            firstPaintTime: t,"
        "            finishLoadTime: t,"
        "            wasFetchedViaSpdy: true,"
        "            wasNpnNegotiated: true,"
        "            npnNegotiatedProtocol: 'h2',"
        "            wasAlternateProtocolAvailable: false,"
        "            connectionInfo: 'h2'"
        "          };"
        "        },"
        "        runtime: {"
        "          OnInstalledReason: { CHROME_UPDATE: 'chrome_update', INSTALL: 'install', SHARED_MODULE_UPDATE: 'shared_module_update', UPDATE: 'update' },"
        "          OnRestartRequiredReason: { APP_UPDATE: 'app_update', OS_UPDATE: 'os_update', PERIODIC: 'periodic' },"
        "          PlatformArch: { ARM: 'arm', ARM64: 'arm64', MIPS: 'mips', MIPS64: 'mips64', X86_32: 'x86-32', X86_64: 'x86-64' },"
        "          PlatformNaclArch: { ARM: 'arm', MIPS: 'mips', MIPS64: 'mips64', X86_32: 'x86-32', X86_64: 'x86-64' },"
        "          PlatformOs: { ANDROID: 'android', CROS: 'cros', LINUX: 'linux', MAC: 'mac', OPENBSD: 'openbsd', WIN: 'win' },"
        "          RequestUpdateCheckStatus: { NO_UPDATE: 'no_update', THROTTLED: 'throttled', UPDATE_AVAILABLE: 'update_available' }"
        "        }"
        "      };"
        "    }"
        "    delete Object.getPrototypeOf(navigator).webdriver;"
        "  } catch(e) {}"
        "  try {"
        "    Object.defineProperty(navigator, 'webdriver', {"
        "      get: function() { return undefined; },"
        "      configurable: true"
        "    });"
        "  } catch(e) {}"
        "})();"
    );

    QWebEngineScript script;
    script.setName(STEALTH_SCRIPT_NAME);
    script.setSourceCode(stealthCode);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);

    m_profile->scripts()->insert(script);
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
