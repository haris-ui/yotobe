#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class QGraphicsOpacityEffect;

class SplashScreen : public QWidget {
    Q_OBJECT

public:
    explicit SplashScreen(QWidget* parent = nullptr);

    void setStatus(const QString& text);
    void finishWithFade();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QWidget* m_cardWidget{nullptr};
    QLabel* m_logoLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_subtitleLabel{nullptr};
    QLabel* m_authorLabel{nullptr};
    QLabel* m_statusLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    bool m_isFadingOut{false};
};
