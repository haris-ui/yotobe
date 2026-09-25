#include "MainWindow.h"
#include <QApplication>
#include <QLocale>
#include <QIcon>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "[INFO] Starting Yotobe Desktop..." << std::endl;

    // 1. Chromium flags:
    //    Rendering & decode — GPU rasterization, zero-copy, accelerated video
    //    Memory limits     — cap renderer processes, disk cache, media cache
    //    Process model     — process-per-site groups sites into fewer renderer processes
    //    Background work   — disable DNS pre-fetch, component updates, background networking
    //    Stealth           — hide automation flags, disable fingerprinting APIs
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            // --- Rendering quality ---
            "--enable-gpu-rasterization "
            "--enable-zero-copy "
            "--ignore-gpu-blocklist "
            "--enable-smooth-scrolling "
            "--enable-accelerated-video-decode "
            "--num-raster-threads=4 "
            // --- Memory / process limits (biggest RAM reduction) ---
            "--renderer-process-limit=2 "
            "--process-per-site "
            "--disk-cache-size=52428800 "
            "--media-cache-size=52428800 "
            "--max-unused-resource-memory-usage-percentage=5 "
            // --- Disable background resource usage ---
            "--disable-background-networking "
            "--disable-component-update "
            "--disable-default-apps "
            // --- Stealth ---
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
