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

DownloadDialog::DownloadDialog(VideoDownloader* downloader,
                               const QUrl& currentVideoUrl,
                               SettingsManager* settings,
                               QWidget* parent)
    : QDialog(parent)
    , m_downloader(downloader)
    , m_settings(settings)
    , m_videoUrl(currentVideoUrl)
{
    setWindowTitle("Yotobe - Video Downloader");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setFixedSize(580, 520);
    setStyleSheet(
        "QDialog { background-color: #141414; color: #f1f1f1; font-family: 'Segoe UI', sans-serif; }"
        "QLabel { color: #d0d0d0; font-size: 12px; font-weight: 500; }"
        "QLineEdit { background-color: #1e1e1e; color: #ffffff; border: 1px solid #333333; border-radius: 6px; padding: 6px 10px; font-size: 12px; }"
        "QLineEdit:focus { border-color: #3ea6ff; }"
        "QComboBox { background-color: #1e1e1e; color: #ffffff; border: 1px solid #333333; border-radius: 6px; padding: 6px 10px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #242424; color: #ffffff; selection-background-color: #383838; }"
        "QListWidget { background-color: #1a1a1a; color: #e0e0e0; border: 1px solid #2e2e2e; border-radius: 6px; padding: 4px; font-size: 12px; }"
        "QListWidget::item { padding: 4px 6px; border-bottom: 1px solid #222; }"
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
    mainLayout->setContentsMargins(20, 18, 20, 16);
    mainLayout->setSpacing(12);

    // Title header
    auto* titleHeader = new QLabel(
        "<span style='font-size:16px;font-weight:700;color:#ffffff;'>Download YouTube Media</span>",
        this);
    mainLayout->addWidget(titleHeader);

    // Backend status badge
    bool hasYtDlp = VideoDownloader::isBackendAvailable();
    bool hasFfmpeg = VideoDownloader::isFfmpegAvailable();
    QString statusBadge = QString(
        "<span style='color:%1;font-size:11px;font-weight:600;'>yt-dlp: %2</span> &nbsp;|&nbsp; "
        "<span style='color:%3;font-size:11px;font-weight:600;'>ffmpeg: %4</span>")
        .arg(hasYtDlp ? "#4ade80" : "#ff4444",
             hasYtDlp ? "Ready" : "Missing",
             hasFfmpeg ? "#4ade80" : "#eab308",
             hasFfmpeg ? "Ready (MP4/MP3 Merging Enabled)" : "Not Detected (Direct Streams Only)");

    m_backendInfoLabel = new QLabel(statusBadge, this);
    mainLayout->addWidget(m_backendInfoLabel);

    // URL row
    auto* urlHeaderLayout = new QHBoxLayout();
    urlHeaderLayout->addWidget(new QLabel("Video URL:", this));
    urlHeaderLayout->addStretch();
    mainLayout->addLayout(urlHeaderLayout);

    auto* urlInputLayout = new QHBoxLayout();
    urlInputLayout->setSpacing(6);

    QString initialUrl = m_videoUrl.toString();
    if (!isDirectVideoUrl(initialUrl)) {
        initialUrl.clear();
    }
    m_urlEdit = new QLineEdit(initialUrl, this);
    m_urlEdit->setPlaceholderText("Paste a YouTube video or Shorts link here...");
    m_urlEdit->setClearButtonEnabled(true);
    urlInputLayout->addWidget(m_urlEdit, 1);

    m_pasteBtn = new QPushButton("Paste", this);
    m_pasteBtn->setFixedHeight(32);
    m_pasteBtn->setStyleSheet("background-color: #272727; color: #f1f1f1; border: 1px solid #3a3a3a; border-radius: 6px; padding: 4px 12px; font-weight: 600;");
    connect(m_pasteBtn, &QPushButton::clicked, this, &DownloadDialog::pasteUrlClicked);
    urlInputLayout->addWidget(m_pasteBtn);
    mainLayout->addLayout(urlInputLayout);

    // Format selection
    auto* formatLayout = new QHBoxLayout();
    formatLayout->setSpacing(10);
    formatLayout->addWidget(new QLabel("Quality / Format:", this));
    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("Best Quality Available (MP4)", static_cast<int>(DownloadFormat::BestVideoAudio));
    m_formatCombo->addItem("1080p Full HD (MP4)",         static_cast<int>(DownloadFormat::Video1080p));
    m_formatCombo->addItem("720p HD (MP4)",              static_cast<int>(DownloadFormat::Video720p));
    m_formatCombo->addItem("480p SD (Fast Download)",     static_cast<int>(DownloadFormat::Video480p));
    m_formatCombo->addItem("Audio Only (MP3)",           static_cast<int>(DownloadFormat::AudioOnlyMP3));
    m_formatCombo->addItem("Audio Only (M4A High Quality)", static_cast<int>(DownloadFormat::AudioOnlyM4A));
    formatLayout->addWidget(m_formatCombo, 1);
    mainLayout->addLayout(formatLayout);

    // Save location — prefer user's configured path, fall back to Movies folder
    auto* destLayout = new QHBoxLayout();
    destLayout->setSpacing(6);
    destLayout->addWidget(new QLabel("Save To:", this));
    QString defaultPath = m_settings
        ? m_settings->defaultDownloadPath()
        : QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    m_destinationEdit = new QLineEdit(defaultPath, this);
    destLayout->addWidget(m_destinationEdit, 1);

    auto* browseBtn = new QPushButton("Browse...", this);
    browseBtn->setFixedHeight(32);
    browseBtn->setStyleSheet("background-color: #272727; color: #f1f1f1; border: 1px solid #3a3a3a; border-radius: 6px; padding: 4px 12px; font-weight: 600;");
    connect(browseBtn, &QPushButton::clicked, this, &DownloadDialog::browseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: none; background-color: #222222; border-radius: 3px; }"
        "QProgressBar::chunk { background-color: #cc0000; border-radius: 3px; }");
    mainLayout->addWidget(m_progressBar);

    // Status label
    m_statusLabel = new QLabel("Ready to download", this);
    m_statusLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);

    // Action buttons (Start, Cancel, Open Folder)
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    m_downloadBtn = new QPushButton("Start Download", this);
    m_downloadBtn->setFixedHeight(34);
    m_downloadBtn->setStyleSheet(
        "QPushButton { background-color: #cc0000; color: #ffffff; font-weight: 700; border: none; border-radius: 6px; padding: 6px 18px; }"
        "QPushButton:hover { background-color: #e60000; }"
        "QPushButton:disabled { background-color: #3a1515; color: #666; }");
    connect(m_downloadBtn, &QPushButton::clicked, this, &DownloadDialog::startDownloadClicked);
    actionLayout->addWidget(m_downloadBtn);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setFixedHeight(34);
    m_cancelBtn->setEnabled(false);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background-color: #272727; color: #f1f1f1; border: 1px solid #383838; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton:hover { background-color: #333333; }"
        "QPushButton:disabled { background-color: #1a1a1a; color: #444; border-color: #242424; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &DownloadDialog::cancelDownloadClicked);
    actionLayout->addWidget(m_cancelBtn);

    m_openFolderBtn = new QPushButton("Open Download Folder", this);
    m_openFolderBtn->setFixedHeight(34);
    m_openFolderBtn->setStyleSheet(
        "QPushButton { background-color: #1a2733; color: #3ea6ff; border: 1px solid #23425e; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton:hover { background-color: #213547; border-color: #33618a; }");
    connect(m_openFolderBtn, &QPushButton::clicked, this, &DownloadDialog::openFolderClicked);
    actionLayout->addWidget(m_openFolderBtn);

    mainLayout->addLayout(actionLayout);

    // Download history
    mainLayout->addWidget(new QLabel("Download History:", this));
    m_historyList = new QListWidget(this);
    m_historyList->setFixedHeight(110);
    mainLayout->addWidget(m_historyList);

    // Footer Author credit
    auto* authorLabel = new QLabel("Made by Muhammad Haris Zubair", this);
    authorLabel->setStyleSheet("color: #e2b714; font-size: 11px; font-weight: 600;");
    authorLabel->setAlignment(Qt::AlignRight);
    mainLayout->addWidget(authorLabel);
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
