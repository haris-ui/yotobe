#include "MainWindow.h"
#include <QApplication>
#include <QLocale>
#include <QIcon>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "[INFO] Starting Yotobe Desktop..." << std::endl;

    // 1. Chromium flags: optimal rendering + English language + full Google auth stealth
    //    Key additions:
    //    - AcceptCHFrame: disables the server-driven Client Hints mechanism entirely
    //    - PrivacySandboxSettings4: disables Topics/Privacy Sandbox fingerprinting APIs
    //    - AutomationControlled + HeadlessMode: prevents embedded-WebView detection flags
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--enable-gpu-rasterization "
            "--enable-zero-copy "
            "--ignore-gpu-blocklist "
            "--disable-blink-features=AutomationControlled "
            "--disable-features=UserAgentClientHint,AcceptCHFrame,PrivacySandboxSettings4 "
            "--lang=en-US "
            "--no-first-run "
            "--no-default-browser-check");

    // 2. Set default application locale to English (United States)
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    QApplication app(argc, argv);

    app.setApplicationName("Yotobe");
    app.setApplicationDisplayName("Yotobe");
    app.setOrganizationName("Yotobe");
    app.setOrganizationDomain("yotobe.local");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":/icons/app_icon.png"));

    MainWindow window;
    window.show();

    return app.exec();
}
