#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include "toast.h"

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

public slots:
    void saveSettings();
    void clearImageCache();

signals:
    void tmdbKeyUpdated();

private:
    QCheckBox *m_gpuToggle;
    QLineEdit *m_tmdbEntry;
    QLineEdit *m_metaLangEntry;
    QLineEdit *m_rclonePathEntry;
    QLineEdit *m_rcloneFlagsEntry;
    QSpinBox *m_mpvCacheSpinBox;
    QComboBox *m_audioLangComboBox;
    QComboBox *m_subLangComboBox;
    QPushButton *m_saveBtn;
    QPushButton *m_clearCacheBtn;
    Toast *m_toast;
};