#include "sidebar.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDir>
#include <QLabel> 
#include <QPropertyAnimation>
#include <QDebug>


SidebarItem::SidebarItem(const QIcon &icon, const QString &text, const QString &path, QWidget *parent)
    : QWidget(parent), m_path(path), m_text(text), m_icon(icon)
{
    setFixedHeight(60); 
    setCursor(Qt::PointingHandCursor);

    m_hoverAnim = new QPropertyAnimation(this, "hoverProgress", this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setStartValue(0.0f);
    m_hoverAnim->setEndValue(1.0f);

    m_activeAnim = new QPropertyAnimation(this, "activeProgress", this);
    m_activeAnim->setDuration(300);
    m_activeAnim->setStartValue(0.0f);
    m_activeAnim->setEndValue(1.0f);
}

void SidebarItem::setActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;
    m_activeAnim->stop();
    m_activeAnim->setStartValue(m_activeProgress);
    m_activeAnim->setEndValue(active ? 1.0f : 0.0f);
    m_activeAnim->start();
}

void SidebarItem::enterEvent(QEnterEvent *) {
    m_hoverAnim->setDirection(QAbstractAnimation::Forward);
    if (m_hoverAnim->state() == QAbstractAnimation::Stopped) m_hoverAnim->start();
}

void SidebarItem::leaveEvent(QEvent *) {
    m_hoverAnim->setDirection(QAbstractAnimation::Backward);
    if (m_hoverAnim->state() == QAbstractAnimation::Stopped) m_hoverAnim->start();
}

void SidebarItem::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) emit clicked(m_path);
}

void SidebarItem::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor hoverBg("#24283b");
    QColor activeAccent("#7aa2f7");
    QColor iconColor = (m_isActive || m_activeProgress > 0.5f) ? activeAccent : QColor("#565f89");

    if (m_hoverProgress > 0.01f) {
        p.setOpacity(m_hoverProgress);
        p.fillRect(rect(), hoverBg);
        p.setOpacity(1.0);
    }

    if (m_activeProgress > 0.01f) {
        float h = 30.0f * m_activeProgress;
        p.setBrush(activeAccent); p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, (height() - h) / 2, 4, h, 2, 2);
    }

    int iconSize = 28;
    float scale = 1.0f + (0.1f * m_hoverProgress);
    
    p.save();
    p.translate(width() / 2, height() / 2);
    p.scale(scale, scale);
    p.translate(-width() / 2, -height() / 2);

    QPixmap pix = m_icon.pixmap(iconSize, iconSize);
    if (!pix.isNull()) {
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
        QPixmap colorized(pix.size()); colorized.fill(Qt::transparent);
        QPainter ip(&colorized);
        ip.setRenderHint(QPainter::Antialiasing);
        ip.setCompositionMode(QPainter::CompositionMode_Source);
        ip.fillRect(colorized.rect(), iconColor);
        ip.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        ip.drawPixmap(0, 0, pix);
        ip.end();
        p.drawPixmap((width() - iconSize) / 2, (height() - iconSize) / 2, colorized);
    }
    p.restore();
}


Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    
    setFixedWidth(80);
    
    setStyleSheet("Sidebar { background-color: #16161e; border-right: 1px solid #1a1b26; }");
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 20, 0, 20);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignTop);

    addLogo();

    layout->addSpacing(20);

    addItem(QIcon(":/icons/home.svg"), "Library", ""); 
    addItem(QIcon(":/icons/dir.svg"), "Browse", ":browse");
    addItem(QIcon(":/icons/drives.svg"), "Drives", ":drives"); 
    addItem(QIcon(":/icons/settings.svg"), "Settings", ":settings");

    layout->addStretch();
    if (!m_items.isEmpty()) m_items.first()->setActive(true);
}

void Sidebar::addLogo() {
    QLabel *logoLabel = new QLabel(this);

    logoLabel->setStyleSheet("background-color: transparent; border: none;");

    QPixmap logo(":/icons/logo.svg");
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("RC");
        logoLabel->setStyleSheet("color: #7aa2f7; font-weight: bold; font-size: 18px; background: transparent;");
    }
    
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setFixedHeight(60);
    layout()->addWidget(logoLabel);
}

void Sidebar::addItem(const QIcon &icon, const QString &text, const QString &path) {
    SidebarItem *item = new SidebarItem(icon, text, path, this);
    layout()->addWidget(item);
    m_items.append(item);
    connect(item, &SidebarItem::clicked, this, [this, item](const QString &p){
        for (auto *i : m_items) i->setActive(i == item);
        emit directorySelected(p);
    });
}