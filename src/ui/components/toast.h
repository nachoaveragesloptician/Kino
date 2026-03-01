#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class Toast : public QWidget
{
    Q_OBJECT
public:
    explicit Toast(QWidget *parent = nullptr);
    void showMessage(const QString &message, int durationMs = 3000);

private:
    QLabel *m_label;
    QTimer *m_timer;
    QPropertyAnimation *m_anim;
};