#include "MainWindow.h"
#include <QApplication>
#include <QLocale>
#include <QIcon>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "[INFO] Starting Yotobe Desktop..." << std::endl;

    // Chromium flags:
    // - GPU rasterization & accelerated video decode for smooth 60fps playback
    // - AutomationControlled disabled natively so navigator.webdriver is false
    // - Standard user environment (clean, unflagged browser instance)
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--enable-gpu-rasterization "
            "--enable-zero-copy "
            "--ignore-gpu-blocklist "
            "--enable-smooth-scrolling "
            "--enable-accelerated-video-decode "
            "--num-raster-threads=4 "
            "--disable-blink-features=AutomationControlled "
            "--lang=en-US "
            "--no-first-run "
            "--no-default-browser-check");

    // Default application locale: English (United States)
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
