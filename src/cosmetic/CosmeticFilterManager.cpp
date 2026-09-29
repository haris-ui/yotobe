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

        // ── 1. window.chrome polyfill ──────────────────────────────────────
        "  try {"
        "    if (!window.chrome) {"
        "      window.chrome = {"
        "        app: {"
        "          isInstalled: false,"
        "          InstallState: { DISABLED:'disabled', INSTALLED:'installed', NOT_INSTALLED:'not_installed' },"
        "          RunningState: { CANNOT_RUN:'cannot_run', READY_TO_RUN:'ready_to_run', RUNNING:'running' }"
        "        },"
        "        csi: function() {},"
        "        loadTimes: function() {"
        "          var t = (window.performance && window.performance.timeOrigin) ? window.performance.timeOrigin/1000 : Date.now()/1000;"
        "          return { requestTime:t, startLoadTime:t, commitLoadTime:t, finishDocumentLoadTime:t,"
        "                   firstPaintTime:t, finishLoadTime:t, wasFetchedViaSpdy:true,"
        "                   wasNpnNegotiated:true, npnNegotiatedProtocol:'h2',"
        "                   wasAlternateProtocolAvailable:false, connectionInfo:'h2' };"
        "        },"
        "        runtime: {"
        "          id: undefined,"
        "          connect: function() { return { onMessage:{addListener:function(){}}, onDisconnect:{addListener:function(){}}, disconnect:function(){}, postMessage:function(){} }; },"
        "          sendMessage: function() {},"
        "          getManifest: function() { return {}; },"
        "          getURL: function(p) { return p; },"
        "          reload: function() {},"
        "          OnInstalledReason: { CHROME_UPDATE:'chrome_update', INSTALL:'install', SHARED_MODULE_UPDATE:'shared_module_update', UPDATE:'update' },"
        "          OnRestartRequiredReason: { APP_UPDATE:'app_update', OS_UPDATE:'os_update', PERIODIC:'periodic' },"
        "          PlatformArch: { ARM:'arm', ARM64:'arm64', MIPS:'mips', MIPS64:'mips64', X86_32:'x86-32', X86_64:'x86-64' },"
        "          PlatformOs: { ANDROID:'android', CROS:'cros', LINUX:'linux', MAC:'mac', OPENBSD:'openbsd', WIN:'win' },"
        "          RequestUpdateCheckStatus: { NO_UPDATE:'no_update', THROTTLED:'throttled', UPDATE_AVAILABLE:'update_available' }"
        "        }"
        "      };"
        "    }"
        "  } catch(e) {}"

        // ── 2. navigator.webdriver = undefined ────────────────────────────
        "  try { delete Object.getPrototypeOf(navigator).webdriver; } catch(e) {}"
        "  try {"
        "    Object.defineProperty(navigator, 'webdriver', { get:function(){ return undefined; }, configurable:true });"
        "  } catch(e) {}"

        // ── 3. navigator.userAgentData (UA-CH) ────────────────────────────
        // Google GlifWebSignIn checks userAgentData.brands for 'Google Chrome'.
        // Without this, the sign-in detects an embedded/non-Chrome browser even
        // when the User-Agent string says Chrome/131.
        "  try {"
        "    if (!navigator.userAgentData) {"
        "      var _brands = ["
        "        { brand:'Google Chrome', version:'131' },"
        "        { brand:'Chromium',      version:'131' },"
        "        { brand:'Not_A Brand',   version:'24'  }"
        "      ];"
        "      var _uaData = {"
        "        brands: _brands,"
        "        mobile: false,"
        "        platform: 'Windows',"
        "        getHighEntropyValues: function(hints) {"
        "          return Promise.resolve({"
        "            brands: _brands,"
        "            mobile: false,"
        "            platform: 'Windows',"
        "            platformVersion: '10.0.0',"
        "            architecture: 'x86',"
        "            bitness: '64',"
        "            model: '',"
        "            uaFullVersion: '131.0.6778.205',"
        "            fullVersionList: ["
        "              { brand:'Google Chrome', version:'131.0.6778.205' },"
        "              { brand:'Chromium',      version:'131.0.6778.205' },"
        "              { brand:'Not_A Brand',   version:'24.0.0.0' }"
        "            ]"
        "          });"
        "        },"
        "        toJSON: function() { return { brands:_brands, mobile:false, platform:'Windows' }; }"
        "      };"
        "      Object.defineProperty(navigator, 'userAgentData', { get:function(){ return _uaData; }, configurable:true });"
        "    }"
        "  } catch(e) {}"

        // ── 4. Permissions API stub ────────────────────────────────────────
        "  try {"
        "    if (navigator.permissions && navigator.permissions.query) {"
        "      var _origQuery = navigator.permissions.query.bind(navigator.permissions);"
        "      navigator.permissions.query = function(params) {"
        "        if (params && (params.name==='notifications' || params.name==='geolocation')) {"
        "          return Promise.resolve({ state:'prompt', onchange:null });"
        "        }"
        "        return _origQuery(params);"
        "      };"
        "    }"
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
