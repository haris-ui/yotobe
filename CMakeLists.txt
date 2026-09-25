cmake_minimum_required(VERSION 3.22)
project(Yotobe VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineWidgets
    WebEngineCore
    Network
)

include_directories(
    src
    src/browser
    src/filtering
    src/cosmetic
    src/downloader
    src/settings
    src/ui
)

set(SOURCES
    src/main.cpp
    src/browser/UrlPolicy.cpp
    src/browser/BrowserView.cpp
    src/browser/NavigationManager.cpp
    src/filtering/FilterStatistics.cpp
    src/filtering/RuleParser.cpp
    src/filtering/RuleMatcher.cpp
    src/filtering/UrlFilterInterceptor.cpp
    src/filtering/FilterManager.cpp
    src/cosmetic/CosmeticFilterManager.cpp
    src/downloader/DownloadItem.cpp
    src/downloader/VideoDownloader.cpp
    src/settings/SettingsManager.cpp
    src/ui/DownloadDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/SplashScreen.cpp
    src/ui/MainWindow.cpp
    resources/resources.qrc
)

set(HEADERS
    src/browser/UrlPolicy.h
    src/browser/BrowserView.h
    src/browser/NavigationManager.h
    src/filtering/FilterStatistics.h
    src/filtering/RuleParser.h
    src/filtering/RuleMatcher.h
    src/filtering/UrlFilterInterceptor.h
    src/filtering/FilterManager.h
    src/cosmetic/CosmeticFilterManager.h
    src/downloader/DownloadItem.h
    src/downloader/VideoDownloader.h
    src/settings/SettingsManager.h
    src/ui/DownloadDialog.h
    src/ui/SettingsDialog.h
    src/ui/SplashScreen.h
    src/ui/MainWindow.h
)

if(WIN32)
    set(RC_SOURCES resources/app.rc)
    add_executable(Yotobe WIN32 ${SOURCES} ${HEADERS} ${RC_SOURCES})
else()
    add_executable(Yotobe ${SOURCES} ${HEADERS})
endif()

target_link_libraries(Yotobe PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
)

if(WIN32)
    target_link_libraries(Yotobe PRIVATE dwmapi)
endif()

if(MSVC)
    target_compile_options(Yotobe PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(Yotobe PRIVATE -Wall -Wextra)
endif()
