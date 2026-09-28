#include "DownloadDialog.h"
#include "VideoDownloader.h"
#include "SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

DownloadDialog::DownloadDialog(VideoDownloader* downloader,
                               const QUrl& currentVideoUrl,
                               SettingsManager* settings,
                               QWidget* parent)
    : QDialog(parent)
    , m_downloader(downloader)
    , m_settings(settings)
    , m_videoUrl(currentVideoUrl)
{
    setWindowTitle("Yotobe - Media Downloader");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setFixedSize(620, 560);

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    COLORREF captionColor = RGB(9, 9, 9);
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    COLORREF borderColor = RGB(38, 38, 38);
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
#endif

    setStyleSheet(
        "QDialog {"
        "  background-color: #090909;"
        "  color: #ffffff;"
        "  font-family: 'GT Walsheim', 'Segoe UI Variable Text', 'Segoe UI', -apple-system, sans-serif;"
        "}"
        "QLabel {"
        "  color: #999999;"
        "  font-size: 13px;"
        "}"
        "QLineEdit {"
        "  background-color: #141414;"
        "  color: #ffffff;"
        "  border: 1px solid #262626;"
        "  border-radius: 10px;"
        "  padding: 8px 12px;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #0099ff;"
        "}"
        "QComboBox {"
        "  background-color: #141414;"
        "  color: #ffffff;"
        "  border: 1px solid #262626;"
        "  border-radius: 10px;"
        "  padding: 8px 12px;"
        "  font-size: 13px;"
        "}"
        "QComboBox:focus {"
        "  border: 1px solid #0099ff;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/icons/chevron_down.svg);"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #1c1c1c;"
        "  color: #ffffff;"
        "  selection-background-color: #262626;"
        "  selection-color: #ffffff;"
        "  border: 1px solid #262626;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
        "QListWidget {"
        "  background-color: #0c0c0c;"
        "  color: #cccccc;"
        "  border: 1px solid #222222;"
        "  border-radius: 12px;"
        "  padding: 6px;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 5px 8px;"
        "  border-bottom: 1px solid #161616;"
        "}"
        "QScrollBar:vertical {"
        "  background: #0c0c0c;"
        "  width: 6px;"
        "  margin: 0;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #262626;"
        "  min-height: 20px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #383838;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
    );

    setupUi();

    if (m_downloader) {
        connect(m_downloader, &VideoDownloader::downloadProgress, this, &DownloadDialog::updateProgress);
        connect(m_downloader, &VideoDownloader::downloadFinished, this, &DownloadDialog::downloadCompleted);
    }
}

bool DownloadDialog::isDirectVideoUrl(const QString& urlStr) const {
    return urlStr.contains("watch?v=") ||
           urlStr.contains("youtu.be/") ||
           urlStr.contains("/shorts/") ||
           urlStr.contains("/live/");
}

void DownloadDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 22, 24, 20);
    mainLayout->setSpacing(14);

    const QString secondaryBtnStyle =
        "QPushButton {"
        "  background-color: #1c1c1c;"
        "  color: #ffffff;"
        "  font-weight: 500;"
        "  font-size: 12px;"
        "  border: 1px solid #262626;"
        "  border-radius: 100px;"
        "  padding: 0 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #242424;"
        "  border-color: #333333;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #181818;"
        "}";

    // ─────────────────────────────────────────────────────────────
    // Title Header & Backend Status
    // ─────────────────────────────────────────────────────────────
    auto* headerLayout = new QHBoxLayout();
    auto* titleHeader = new QLabel(
        "<span style='font-size:18px;font-weight:600;color:#ffffff;letter-spacing:-0.4px;'>Download Media</span>",
        this);
    headerLayout->addWidget(titleHeader);
    headerLayout->addStretch();

    bool hasYtDlp = VideoDownloader::isBackendAvailable();
    bool hasFfmpeg = VideoDownloader::isFfmpegAvailable();

    QString ytdlpBadge = hasYtDlp
        ? "<span style='background-color:#0f2015;color:#22c55e;border:1px solid #193d25;border-radius:100px;padding:3px 10px;font-size:11px;font-weight:600;'>yt-dlp: Ready</span>"
        : "<span style='background-color:#291010;color:#ef4444;border:1px solid #4a1c1c;border-radius:100px;padding:3px 10px;font-size:11px;font-weight:600;'>yt-dlp: Missing</span>";

    QString ffmpegBadge = hasFfmpeg
        ? "<span style='background-color:#0f2015;color:#22c55e;border:1px solid #193d25;border-radius:100px;padding:3px 10px;font-size:11px;font-weight:600;'>ffmpeg: Ready</span>"
        : "<span style='background-color:#261c0c;color:#f59e0b;border:1px solid #3d2d14;border-radius:100px;padding:3px 10px;font-size:11px;font-weight:600;'>ffmpeg: Not Detected</span>";

    m_backendInfoLabel = new QLabel(QString("%1 &nbsp; %2").arg(ytdlpBadge, ffmpegBadge), this);
    headerLayout->addWidget(m_backendInfoLabel);
    mainLayout->addLayout(headerLayout);

    // ─────────────────────────────────────────────────────────────
    // Video URL input
    // ─────────────────────────────────────────────────────────────
    auto* urlLabel = new QLabel("VIDEO URL", this);
    urlLabel->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    mainLayout->addWidget(urlLabel);

    auto* urlInputLayout = new QHBoxLayout();
    urlInputLayout->setSpacing(8);

    QString initialUrl = m_videoUrl.toString();
    if (!isDirectVideoUrl(initialUrl)) {
        initialUrl.clear();
    }
    m_urlEdit = new QLineEdit(initialUrl, this);
    m_urlEdit->setPlaceholderText("Paste a YouTube video or Shorts link here...");
    m_urlEdit->setClearButtonEnabled(true);
    urlInputLayout->addWidget(m_urlEdit, 1);

    m_pasteBtn = new QPushButton("Paste", this);
    m_pasteBtn->setFixedHeight(36);
    m_pasteBtn->setStyleSheet(secondaryBtnStyle);
    connect(m_pasteBtn, &QPushButton::clicked, this, &DownloadDialog::pasteUrlClicked);
    urlInputLayout->addWidget(m_pasteBtn);
    mainLayout->addLayout(urlInputLayout);

    // ─────────────────────────────────────────────────────────────
    // Format Selection
    // ─────────────────────────────────────────────────────────────
    auto* formatLabel = new QLabel("FORMAT & RESOLUTION", this);
    formatLabel->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    mainLayout->addWidget(formatLabel);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("Best Quality Available (MP4)", static_cast<int>(DownloadFormat::BestVideoAudio));
    m_formatCombo->addItem("1080p Full HD (MP4)",         static_cast<int>(DownloadFormat::Video1080p));
    m_formatCombo->addItem("720p HD (MP4)",              static_cast<int>(DownloadFormat::Video720p));
    m_formatCombo->addItem("480p SD (Fast Download)",     static_cast<int>(DownloadFormat::Video480p));
    m_formatCombo->addItem("Audio Only (MP3)",           static_cast<int>(DownloadFormat::AudioOnlyMP3));
    m_formatCombo->addItem("Audio Only (M4A High Quality)", static_cast<int>(DownloadFormat::AudioOnlyM4A));
    mainLayout->addWidget(m_formatCombo);

    // ─────────────────────────────────────────────────────────────
    // Save Location
    // ─────────────────────────────────────────────────────────────
    auto* destLabel = new QLabel("SAVE DESTINATION", this);
    destLabel->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px;");
    mainLayout->addWidget(destLabel);

    auto* destLayout = new QHBoxLayout();
    destLayout->setSpacing(8);

    QString defaultPath = m_settings
        ? m_settings->defaultDownloadPath()
        : QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    m_destinationEdit = new QLineEdit(defaultPath, this);
    destLayout->addWidget(m_destinationEdit, 1);

    auto* browseBtn = new QPushButton("Browse...", this);
    browseBtn->setFixedHeight(36);
    browseBtn->setStyleSheet(secondaryBtnStyle);
    connect(browseBtn, &QPushButton::clicked, this, &DownloadDialog::browseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // ─────────────────────────────────────────────────────────────
    // Progress Bar & Status
    // ─────────────────────────────────────────────────────────────
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: none;"
        "  background-color: #1c1c1c;"
        "  border-radius: 2px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #0099ff;"
        "  border-radius: 2px;"
        "}");
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Ready to download", this);
    m_statusLabel->setStyleSheet("color: #888888; font-size: 12px; font-weight: 500;");
    mainLayout->addWidget(m_statusLabel);

    // ─────────────────────────────────────────────────────────────
    // Action Buttons Row (Start Download, Cancel, Open Folder)
    // ─────────────────────────────────────────────────────────────
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(10);

    // Signature Framer primary white pill button
    m_downloadBtn = new QPushButton("Start Download", this);
    m_downloadBtn->setFixedHeight(36);
    m_downloadBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #000000;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  border-radius: 100px;"
        "  padding: 0 24px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e5e5e5;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #cccccc;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #262626;"
        "  color: #666666;"
        "}");
    connect(m_downloadBtn, &QPushButton::clicked, this, &DownloadDialog::startDownloadClicked);
    actionLayout->addWidget(m_downloadBtn);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setFixedHeight(36);
    m_cancelBtn->setEnabled(false);
    m_cancelBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1c1c1c;"
        "  color: #ffffff;"
        "  font-weight: 500;"
        "  font-size: 13px;"
        "  border: 1px solid #262626;"
        "  border-radius: 100px;"
        "  padding: 0 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #242424;"
        "  border-color: #333333;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #181818;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #141414;"
        "  color: #444444;"
        "  border-color: #1a1a1a;"
        "}");
    connect(m_cancelBtn, &QPushButton::clicked, this, &DownloadDialog::cancelDownloadClicked);
    actionLayout->addWidget(m_cancelBtn);

    m_openFolderBtn = new QPushButton("Open Folder", this);
    m_openFolderBtn->setFixedHeight(36);
    m_openFolderBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #0d2238;"
        "  color: #0099ff;"
        "  font-weight: 500;"
        "  font-size: 13px;"
        "  border: 1px solid #1a3c61;"
        "  border-radius: 100px;"
        "  padding: 0 18px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #13304f;"
        "  border-color: #245285;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0b1a2b;"
        "}");
    connect(m_openFolderBtn, &QPushButton::clicked, this, &DownloadDialog::openFolderClicked);
    actionLayout->addWidget(m_openFolderBtn);

    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // ─────────────────────────────────────────────────────────────
    // Download History List
    // ─────────────────────────────────────────────────────────────
    auto* historyLabel = new QLabel("RECENT DOWNLOADS", this);
    historyLabel->setStyleSheet("color: #666666; font-size: 11px; font-weight: 600; letter-spacing: 0.8px; margin-top: 2px;");
    mainLayout->addWidget(historyLabel);

    m_historyList = new QListWidget(this);
    m_historyList->setFixedHeight(105);
    mainLayout->addWidget(m_historyList);

    // ─────────────────────────────────────────────────────────────
    // Footer Credit
    // ─────────────────────────────────────────────────────────────
    auto* footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(4, 4, 4, 0);

    auto* authorBadge = new QLabel(
        "<span style='color: #777777;'>Made by</span> <span style='color: #ffffff; font-weight: 600;'>Muhammad Haris Zubair</span>",
        this);
    authorBadge->setStyleSheet(
        "background-color: #141414;"
        "border: 1px solid #262626;"
        "border-radius: 100px;"
        "padding: 4px 12px;"
        "font-size: 11px;");
    footerLayout->addWidget(authorBadge);

    footerLayout->addStretch();
    mainLayout->addLayout(footerLayout);
}

void DownloadDialog::pasteUrlClicked() {
    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard) {
        QString text = clipboard->text().trimmed();
        if (!text.isEmpty()) {
            m_urlEdit->setText(text);
        }
    }
}

void DownloadDialog::browseDestination() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Download Folder", m_destinationEdit->text());
    if (!dir.isEmpty()) {
        m_destinationEdit->setText(dir);
    }
}

void DownloadDialog::startDownloadClicked() {
    QString urlStr = m_urlEdit->text().trimmed();
    QUrl url(urlStr);

    if (!url.isValid() || !isDirectVideoUrl(urlStr)) {
        QMessageBox::warning(
            this,
            "Invalid YouTube Video Link",
            "Please paste a direct YouTube video or Shorts link.\n\nExample: https://www.youtube.com/watch?v=...\nor https://youtu.be/...");
        return;
    }

    if (!VideoDownloader::isBackendAvailable()) {
        QMessageBox::warning(
            this,
            "Backend Required",
            "yt-dlp was not found on your system.\n\nPlease place yt-dlp.exe in the Yotobe folder or install it via winget (winget install yt-dlp).");
        return;
    }

    m_downloadBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Starting download process...");

    DownloadFormat fmt = static_cast<DownloadFormat>(m_formatCombo->currentData().toInt());
    m_downloader->startDownload(url, m_destinationEdit->text().trimmed(), fmt);
}

void DownloadDialog::cancelDownloadClicked() {
    if (m_downloader) {
        m_downloader->cancelCurrentDownload();
    }
}

void DownloadDialog::openFolderClicked() {
    QString folderPath = m_destinationEdit->text().trimmed();
    if (QFileInfo::exists(folderPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    }
}

void DownloadDialog::updateProgress(DownloadItem* /*item*/, int percent, const QString& status) {
    m_progressBar->setValue(percent);
    m_statusLabel->setText(status);
}

void DownloadDialog::downloadCompleted(DownloadItem* item, bool success, const QString& msg) {
    m_downloadBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_statusLabel->setText(msg);

    if (item) {
        QString title = item->title().trimmed();
        if (title.isEmpty()) {
            title = item->videoUrl().toString();
        }
        m_historyList->addItem(QString("[%1] %2").arg(success ? "Done" : "Failed", title));
        m_historyList->scrollToBottom();
    }
}
