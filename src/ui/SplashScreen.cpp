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

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Central Card Widget
    m_cardWidget = new QWidget(this);
    m_cardWidget->setFixedSize(460, 360);
    m_cardWidget->setStyleSheet(
        "background-color: #161616;"
        "border-radius: 18px;"
        "border: 1px solid #282828;"
    );

    auto cardLayout = new QVBoxLayout(m_cardWidget);
    cardLayout->setContentsMargins(36, 36, 36, 32);
    cardLayout->setSpacing(12);

    // 1. Logo Display
    m_logoLabel = new QLabel(m_cardWidget);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setStyleSheet("border: none; background: transparent;");

    QPixmap rawLogo(":/icons/app_icon.png");
    if (!rawLogo.isNull()) {
        QPixmap scaledLogo = rawLogo.scaled(104, 104, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPixmap roundedLogo(104, 104);
        roundedLogo.fill(Qt::transparent);
        QPainter painter(&roundedLogo);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(0, 0, 104, 104, 22, 22);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledLogo);
        painter.end();

        m_logoLabel->setPixmap(roundedLogo);
    }
    cardLayout->addWidget(m_logoLabel);

    // 2. Title "Yotobe."
    m_titleLabel = new QLabel(m_cardWidget);
    m_titleLabel->setText("<span style='color: #ffffff; font-weight: 800; font-size: 30px; letter-spacing: -0.5px;'>Yotobe</span><span style='color: #ff0000; font-size: 30px; font-weight: 800;'>.</span>");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("border: none; background: transparent;");
    cardLayout->addWidget(m_titleLabel);

    // 3. Subtitle
    m_subtitleLabel = new QLabel("Dedicated YouTube Desktop Application", m_cardWidget);
    m_subtitleLabel->setStyleSheet("color: #888888; font-size: 13px; font-weight: 500; border: none; background: transparent;");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_subtitleLabel);

    // 4. Author Credits
    m_authorLabel = new QLabel("Made by Muhammad Haris Zubair", m_cardWidget);
    m_authorLabel->setStyleSheet("color: #e2b714; font-size: 13px; font-weight: 600; letter-spacing: 0.4px; padding: 2px; border: none; background: transparent;");
    m_authorLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_authorLabel);

    cardLayout->addStretch();

    // 5. Glowing Indeterminate Progress Bar
    m_progressBar = new QProgressBar(m_cardWidget);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setRange(0, 0); // Indeterminate animated sweep
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #242424; border-radius: 2px; border: none; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff0000, stop:1 #e2b714); border-radius: 2px; }"
    );
    cardLayout->addWidget(m_progressBar);

    // 6. Status text
    m_statusLabel = new QLabel("Initializing environment...", m_cardWidget);
    m_statusLabel->setStyleSheet("color: #606060; font-size: 11px; border: none; background: transparent;");
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

    // Parent nullptr: DeleteWhenStopped has sole ownership — avoids parent/child
    // deletion order conflict when widget's deleteLater() and animation cleanup
    // both fire in the same event-loop cycle.
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
    painter.fillRect(rect(), QColor(15, 15, 15));
}
