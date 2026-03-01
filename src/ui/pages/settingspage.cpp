#include "settingspage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_toast = new Toast(this);

    QWidget *headerContainer = new QWidget(this);
    headerContainer->setFixedHeight(100);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(40, 40, 40, 20);
    headerLayout->setSpacing(20);
    headerLayout->setAlignment(Qt::AlignVCenter);

    QLabel *header = new QLabel("Settings", headerContainer);
    header->setStyleSheet("font-size: 32px; font-weight: 800; color: #7aa2f7; background: transparent; border: none;");
    headerLayout->addWidget(header);
    headerLayout->addStretch();
    layout->addWidget(headerContainer);

    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(40, 0, 40, 40);
    contentLayout->setSpacing(25);

    QSettings settings("Kino", "AppConfig");

    m_gpuToggle = new QCheckBox("Enable Hardware Acceleration (GPU)", contentWidget);
    m_gpuToggle->setStyleSheet("color: #c0caf5; font-size: 16px; font-weight: bold;");
    m_gpuToggle->setCursor(Qt::PointingHandCursor);
    m_gpuToggle->setChecked(settings.value("gpu_accel", true).toBool());
    contentLayout->addWidget(m_gpuToggle);

    QString inputStyle = R"(
        QWidget { 
            background-color: rgba(30, 30, 46, 0.7); 
            color: white; 
            border: 1px solid #414868; 
            border-radius: 8px; 
            padding: 0 15px; 
        }
        QWidget:focus { border: 1px solid #7aa2f7; }
    )";

    QLabel *tmdbLabel = new QLabel("TMDB API Key (Required for Posters)", contentWidget);
    tmdbLabel->setStyleSheet("color: #c0caf5; font-size: 16px; font-weight: bold;");
    contentLayout->addWidget(tmdbLabel);

    m_tmdbEntry = new QLineEdit(contentWidget);
    m_tmdbEntry->setPlaceholderText("Enter your TMDB API Key...");
    m_tmdbEntry->setFixedHeight(42);
    m_tmdbEntry->setStyleSheet(inputStyle);
    m_tmdbEntry->setText(settings.value("tmdb_api_key", "").toString());
    contentLayout->addWidget(m_tmdbEntry);

    QLabel *rcloneLabel = new QLabel("Rclone Config Path", contentWidget);
    rcloneLabel->setStyleSheet("color: #c0caf5; font-size: 16px; font-weight: bold;");
    contentLayout->addWidget(rcloneLabel);

    m_rclonePathEntry = new QLineEdit(contentWidget);
    #ifdef Q_OS_WIN
        QString defaultConfig = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/rclone/rclone.conf";
    #else
        QString defaultConfig = QDir::homePath() + "/.config/rclone/rclone.conf";
    #endif
    m_rclonePathEntry->setText(settings.value("rclone_conf_path", defaultConfig).toString());
    m_rclonePathEntry->setFixedHeight(42);
    m_rclonePathEntry->setStyleSheet(inputStyle);
    contentLayout->addWidget(m_rclonePathEntry);

    QLabel *cacheLabel = new QLabel("MPV Demuxer Cache Size (MB)", contentWidget);
    cacheLabel->setStyleSheet("color: #c0caf5; font-size: 16px; font-weight: bold;");
    contentLayout->addWidget(cacheLabel);

    m_mpvCacheSpinBox = new QSpinBox(contentWidget);
    m_mpvCacheSpinBox->setRange(0, 8192); 
    m_mpvCacheSpinBox->setValue(settings.value("mpv_cache_mb", 150).toInt()); 
    m_mpvCacheSpinBox->setFixedHeight(42);
    m_mpvCacheSpinBox->setStyleSheet(inputStyle + "QSpinBox::up-button, QSpinBox::down-button { width: 0px; }"); 
    contentLayout->addWidget(m_mpvCacheSpinBox);

    m_saveBtn = new QPushButton("Save Settings", contentWidget);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setFixedHeight(45);
    m_saveBtn->setFixedWidth(200);
    m_saveBtn->setStyleSheet(R"(
        QPushButton { 
            background: #7aa2f7; 
            color: #1a1b26; 
            border-radius: 8px; 
            font-weight: bold; 
            font-size: 16px; 
            border: none;
        }
        QPushButton:hover { background: #8db0f8; }
        QPushButton:pressed { background: #6b93e8; }
    )");
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsPage::saveSettings);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addStretch();
    contentLayout->addLayout(btnLayout);

    contentLayout->addStretch();

    QLabel *footer = new QLabel("Changes to GPU or Cache settings require a player restart.\nPosters will update on next library rescan.", contentWidget);
    footer->setStyleSheet("color: #565f89; font-style: italic;");
    contentLayout->addWidget(footer);

    layout->addWidget(contentWidget, 1);
}

void SettingsPage::saveSettings() {
    QSettings settings("Kino", "AppConfig");
    
    settings.setValue("gpu_accel", m_gpuToggle->isChecked());
    settings.setValue("tmdb_api_key", m_tmdbEntry->text().trimmed());
    settings.setValue("rclone_conf_path", m_rclonePathEntry->text().trimmed());
    settings.setValue("mpv_cache_mb", m_mpvCacheSpinBox->value());
    
    m_toast->showMessage("Settings saved successfully!", 2500);
    
    emit tmdbKeyUpdated();
}