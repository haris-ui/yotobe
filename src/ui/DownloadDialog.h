#pragma once

#include <QDialog>
#include <QUrl>

class VideoDownloader;
class QLineEdit;
class QComboBox;
class QProgressBar;
class QLabel;
class QPushButton;
class QListWidget;

class DownloadDialog : public QDialog {
    Q_OBJECT
public:
    explicit DownloadDialog(VideoDownloader* downloader, const QUrl& currentVideoUrl, QWidget* parent = nullptr);

private slots:
    void browseDestination();
    void startDownloadClicked();
    void cancelDownloadClicked();
    void openFolderClicked();
    void pasteUrlClicked();
    void updateProgress(class DownloadItem* item, int percent, const QString& status);
    void downloadCompleted(class DownloadItem* item, bool success, const QString& msg);

private:
    void setupUi();
    bool isDirectVideoUrl(const QString& urlStr) const;

    VideoDownloader* m_downloader{nullptr};
    QUrl m_videoUrl;

    QLineEdit*    m_urlEdit{nullptr};
    QPushButton*  m_pasteBtn{nullptr};
    QLineEdit*    m_destinationEdit{nullptr};
    QComboBox*    m_formatCombo{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel*       m_statusLabel{nullptr};
    QLabel*       m_backendInfoLabel{nullptr};
    QPushButton*  m_downloadBtn{nullptr};
    QPushButton*  m_cancelBtn{nullptr};
    QPushButton*  m_openFolderBtn{nullptr};
    QListWidget*  m_historyList{nullptr};
};
