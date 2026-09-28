#include "SplashScreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Central Card Widget — Framer surface-1 (#141414) on pure canvas (#090909)
    m_cardWidget = new QWidget(this);
    m_cardWidget->setFixedSize(460, 370);
    m_cardWidget->setStyleSheet(
        "background-color: #141414;"
        "border-radius: 22px;"
        "border: 1px solid #262626;"
    );

    auto* cardLayout = new QVBoxLayout(m_cardWidget);
    cardLayout->setContentsMargins(36, 36, 36, 32);
    cardLayout->setSpacing(12);

    // 1. Logo Display
    m_logoLabel = new QLabel(m_cardWidget);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setStyleSheet("border: none; background: transparent;");

    QPixmap rawLogo(":/icons/app_icon.png");
    if (!rawLogo.isNull()) {
        QPixmap scaledLogo = rawLogo.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPixmap roundedLogo(96, 96);
        roundedLogo.fill(Qt::transparent);
        QPainter painter(&roundedLogo);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(0, 0, 96, 96, 20, 20);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledLogo);
        painter.end();

        m_logoLabel->setPixmap(roundedLogo);
    }
    cardLayout->addWidget(m_logoLabel);

    // 2. Title "Yotobe" — Framer display-md typography
    m_titleLabel = new QLabel(m_cardWidget);
    m_titleLabel->setText(
        "<span style='color: #ffffff; font-weight: 700; font-size: 28px; letter-spacing: -1.0px;'>Yotobe</span>");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("border: none; background: transparent;");
    cardLayout->addWidget(m_titleLabel);

    // 3. Subtitle — Framer body-sm typography in ink-muted
    m_subtitleLabel = new QLabel(
        "<span style='color: #888888; font-size: 13px; font-weight: 400; letter-spacing: -0.15px;'>Minimalist Desktop Client for YouTube</span>",
        m_cardWidget);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("border: none; background: transparent;");
    cardLayout->addWidget(m_subtitleLabel);

    // 4. Author Badge — Framer pill chip
    m_authorLabel = new QLabel(
        "<span style='background-color: #1c1c1c; color: #888888; border: 1px solid #282828; border-radius: 100px; padding: 4px 14px; font-size: 11px; font-weight: 500;'>"
        "Engineered by <strong style='color: #ffffff; font-weight: 600;'>Muhammad Haris Zubair</strong></span>",
        m_cardWidget);
    m_authorLabel->setAlignment(Qt::AlignCenter);
    m_authorLabel->setStyleSheet("border: none; background: transparent; margin-top: 4px;");
    cardLayout->addWidget(m_authorLabel);

    cardLayout->addStretch();

    // 5. Hairline Progress Bar — Framer accent violet/blue gradient
    m_progressBar = new QProgressBar(m_cardWidget);
    m_progressBar->setFixedHeight(3);
    m_progressBar->setRange(0, 0); // Indeterminate animated sweep
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #1c1c1c; border-radius: 1.5px; border: none; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #6a4cf5, stop:0.5 #0099ff, stop:1 #6a4cf5); border-radius: 1.5px; }"
    );
    cardLayout->addWidget(m_progressBar);

    // 6. Status text
    m_statusLabel = new QLabel("Initializing environment...", m_cardWidget);
    m_statusLabel->setStyleSheet("color: #555555; font-size: 11px; font-weight: 500; border: none; background: transparent;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(m_cardWidget);

    // Setup Opacity Effect for smooth in-window fade-out
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(1.0);
}

void SplashScreen::setStatus(const QString& text) {
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
}

void SplashScreen::finishWithFade() {
    if (m_isFadingOut) {
        return;
    }
    m_isFadingOut = true;

    auto anim = new QPropertyAnimation(m_opacityEffect, "opacity", nullptr);
    anim->setDuration(350);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        hide();
        deleteLater();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SplashScreen::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    // Framer canvas: #090909
    painter.fillRect(rect(), QColor(9, 9, 9));
}
