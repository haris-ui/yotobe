#include "DownloadDialog.h"
#include "VideoDownloader.h"
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

DownloadDialog::DownloadDialog(VideoDownloader* downloader, const QUrl& currentVideoUrl, QWidget* parent)
    : QDialog(parent)
    , m_downloader(downloader)
    , m_videoUrl(currentVideoUrl)
{
    setWindowTitle("Yotobe - Video Downloader");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setMinimumSize(560, 420);
    setupUi();

    if (m_downloader) {
        connect(m_downloader, &VideoDownloader::downloadProgress, this, &DownloadDialog::updateProgress);
        connect(m_downloader, &VideoDownloader::downloadFinished, this, &DownloadDialog::downloadCompleted);
    }
}

void DownloadDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);

    // URL input
    auto urlLayout = new QHBoxLayout();
    urlLayout->addWidget(new QLabel("YouTube URL:"));
    m_urlEdit = new QLineEdit(m_videoUrl.toString());
    urlLayout->addWidget(m_urlEdit);
    mainLayout->addLayout(urlLayout);

    // Format selection
    auto formatLayout = new QHBoxLayout();
    formatLayout->addWidget(new QLabel("Format / Quality:"));
    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("Best Available Quality", static_cast<int>(DownloadFormat::BestVideoAudio));
    m_formatCombo->addItem("1080p Full HD", static_cast<int>(DownloadFormat::Video1080p));
    m_formatCombo->addItem("720p HD", static_cast<int>(DownloadFormat::Video720p));
    m_formatCombo->addItem("Audio Only (MP3)", static_cast<int>(DownloadFormat::AudioOnlyMP3));
    formatLayout->addWidget(m_formatCombo);
    mainLayout->addLayout(formatLayout);

    // Save Location
    auto destLayout = new QHBoxLayout();
    destLayout->addWidget(new QLabel("Save To:"));
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    m_destinationEdit = new QLineEdit(defaultPath);
    destLayout->addWidget(m_destinationEdit);
    auto browseBtn = new QPushButton("Browse...");
    connect(browseBtn, &QPushButton::clicked, this, &DownloadDialog::browseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // Progress
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(m_statusLabel);

    // Download Button
    auto btnLayout = new QHBoxLayout();
    m_downloadBtn = new QPushButton("Start Download");
    m_downloadBtn->setStyleSheet("background-color: #cc0000; color: white; font-weight: bold; padding: 8px; border-radius: 4px;");
    connect(m_downloadBtn, &QPushButton::clicked, this, &DownloadDialog::startDownloadClicked);
    btnLayout->addWidget(m_downloadBtn);
    mainLayout->addLayout(btnLayout);

    // History
    mainLayout->addWidget(new QLabel("Download History:"));
    m_historyList = new QListWidget();
    mainLayout->addWidget(m_historyList);

    auto authorLabel = new QLabel("Made by Muhammad Haris Zubair", this);
    authorLabel->setStyleSheet("color: #e2b714; font-size: 11px; font-weight: 600; padding-top: 4px;");
    authorLabel->setAlignment(Qt::AlignRight);
    mainLayout->addWidget(authorLabel);
}

void DownloadDialog::browseDestination() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Download Directory", m_destinationEdit->text());
    if (!dir.isEmpty()) {
        m_destinationEdit->setText(dir);
    }
}

void DownloadDialog::startDownloadClicked() {
    QUrl url(m_urlEdit->text().trimmed());
    if (!url.isValid() || (!url.toString().contains("youtube.com") &&
                           !url.toString().contains("youtu.be"))) {
        QMessageBox::warning(this, "Invalid URL", "Please enter a valid YouTube video URL.");
        return;
    }

    if (!VideoDownloader::isBackendAvailable()) {
        QMessageBox::warning(this, "Backend Required",
                             "yt-dlp was not found on your system.\n\nPlease place yt-dlp.exe in the Yotobe directory or install it via winget (winget install yt-dlp).");
        return;
    }

    m_downloadBtn->setEnabled(false);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Starting download...");

    DownloadFormat fmt = static_cast<DownloadFormat>(m_formatCombo->currentData().toInt());
    m_downloader->startDownload(url, m_destinationEdit->text().trimmed(), fmt);
}

void DownloadDialog::updateProgress(DownloadItem* /*item*/, int percent, const QString& status) {
    m_progressBar->setValue(percent);
    m_statusLabel->setText(status);
}

void DownloadDialog::downloadCompleted(DownloadItem* item, bool success, const QString& msg) {
    m_downloadBtn->setEnabled(true);
    m_statusLabel->setText(msg);
    if (item) {
        m_historyList->addItem(QString("[%1] %2").arg(success ? "Done" : "Failed", item->title()));
    }
}
