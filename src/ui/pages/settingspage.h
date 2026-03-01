#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include "toast.h"

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

public slots:
    void saveSettings();

signals:
    void tmdbKeyUpdated();

private:
    QCheckBox *m_gpuToggle;
    QLineEdit *m_tmdbEntry;
    QLineEdit *m_rclonePathEntry;
    QSpinBox *m_mpvCacheSpinBox;
    QPushButton *m_saveBtn;
    Toast *m_toast;
};