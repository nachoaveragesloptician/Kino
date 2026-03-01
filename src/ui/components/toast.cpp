#include "toast.h"
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QApplication>

Toast::Toast(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setStyleSheet(R"(
        QWidget {
            background-color: #1f2335;
            border: 1px solid #7aa2f7;
            border-radius: 8px;
        }
        QLabel {
            color: #c0caf5;
            font-weight: bold;
            font-size: 14px;
            padding: 8px 12px;
            border: none;
            background: transparent;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_label);

    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(eff);

    m_anim = new QPropertyAnimation(eff, "opacity", this);
    m_anim->setDuration(300);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    
    connect(m_timer, &QTimer::timeout, [this, eff](){
        m_anim->setStartValue(1.0);
        m_anim->setEndValue(0.0);
        connect(m_anim, &QPropertyAnimation::finished, this, &Toast::hide);
        m_anim->start();
    });
}

void Toast::showMessage(const QString &message, int durationMs)
{
    m_label->setText(message);
    adjustSize();

    if (parentWidget()) {
        QPoint bottom = parentWidget()->rect().bottomRight();
        move(bottom.x() - width() - 30, bottom.y() - height() - 30);
    }

    show();
    raise();

    m_anim->stop();
    m_anim->setStartValue(0.0);
    m_anim->setEndValue(1.0);
    m_anim->disconnect(this, SLOT(hide())); 
    m_anim->start();

    m_timer->start(durationMs);
}