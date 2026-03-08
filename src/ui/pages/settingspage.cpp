#include "settingspage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QFileDialog>
#include <QScrollArea>

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

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
        QScrollBar::handle:vertical { background: #414868; border-radius: 3px; }
        QScrollBar::handle:vertical:hover { background: #7aa2f7; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");

    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(40, 0, 40, 40);
    scrollLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    QWidget *contentWidget = new QWidget(scrollContent);
    contentWidget->setMaximumWidth(700);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(20);

    QSettings settings("Kino", "AppConfig");

    QString inputStyle = R"(
        QWidget { 
            background-color: rgba(30, 30, 46, 0.7); 
            color: white; 
            border: 1px solid #414868; 
            border-radius: 8px; 
            padding: 0 15px; 
        }
        QWidget:focus { border: 1px solid #7aa2f7; }
        QComboBox::drop-down { border: none; }
        QComboBox::down-arrow { image: none; }
        QComboBox QAbstractItemView { background: #1a1b26; color: #c0caf5; selection-background-color: #7aa2f7; selection-color: #1a1b26; border: 1px solid #414868; border-radius: 8px; outline: none; }
    )";

    QString headerStyle = "color: #565f89; font-size: 13px; font-weight: bold; letter-spacing: 1px; margin-top: 15px;";
    QString labelStyle = "color: #c0caf5; font-size: 15px; font-weight: bold;";

    QLabel *playerSectionLabel = new QLabel("PLAYER & PLAYBACK", contentWidget);
    playerSectionLabel->setStyleSheet(headerStyle);
    contentLayout->addWidget(playerSectionLabel);

    m_gpuToggle = new QCheckBox("Enable Hardware Acceleration (GPU)", contentWidget);
    m_gpuToggle->setStyleSheet("color: #c0caf5; font-size: 15px; font-weight: bold;");
    m_gpuToggle->setCursor(Qt::PointingHandCursor);
    m_gpuToggle->setChecked(settings.value("gpu_accel", true).toBool());
    contentLayout->addWidget(m_gpuToggle);

    QLabel *cacheLabel = new QLabel("MPV Demuxer Cache Size (MB)", contentWidget);
    cacheLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(cacheLabel);

    m_mpvCacheSpinBox = new QSpinBox(contentWidget);
    m_mpvCacheSpinBox->setRange(0, 8192); 
    m_mpvCacheSpinBox->setValue(settings.value("mpv_cache_mb", 150).toInt()); 
    m_mpvCacheSpinBox->setFixedHeight(42);
    m_mpvCacheSpinBox->setStyleSheet(inputStyle + "QSpinBox::up-button, QSpinBox::down-button { width: 0px; }"); 
    contentLayout->addWidget(m_mpvCacheSpinBox);

    QList<QPair<QString, QString>> langs = {
        {"Auto / System Default", "auto"},
        {"English", "eng,en"},
        {"Japanese", "jpn,ja"},
        {"Korean", "kor,ko"},
        {"Spanish", "spa,es"},
        {"French", "fre,fra,fr"},
        {"German", "ger,deu,de"},
        {"Italian", "ita,it"},
        {"Hindi", "hin,hi"},
        {"Chinese", "chi,zho,zh"},
        {"Portuguese", "por,pt"},
        {"Russian", "rus,ru"},
        {"Arabic", "ara,ar"}
    };

    QString savedAudio = settings.value("audio_lang", "auto").toString();
    QString savedSub = settings.value("sub_lang", "eng,en").toString();

    QLabel *audioLangLabel = new QLabel("Preferred Audio Language", contentWidget);
    audioLangLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(audioLangLabel);

    m_audioLangComboBox = new QComboBox(contentWidget);
    m_audioLangComboBox->setFixedHeight(42);
    m_audioLangComboBox->setStyleSheet(inputStyle);
    m_audioLangComboBox->setCursor(Qt::PointingHandCursor);
    for (int i = 0; i < langs.size(); ++i) {
        m_audioLangComboBox->addItem(langs[i].first, langs[i].second);
        if (langs[i].second == savedAudio) m_audioLangComboBox->setCurrentIndex(i);
    }
    contentLayout->addWidget(m_audioLangComboBox);

    QLabel *subLangLabel = new QLabel("Preferred Subtitle Language", contentWidget);
    subLangLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(subLangLabel);

    m_subLangComboBox = new QComboBox(contentWidget);
    m_subLangComboBox->setFixedHeight(42);
    m_subLangComboBox->setStyleSheet(inputStyle);
    m_subLangComboBox->setCursor(Qt::PointingHandCursor);
    for (int i = 0; i < langs.size(); ++i) {
        m_subLangComboBox->addItem(langs[i].first, langs[i].second);
        if (langs[i].second == savedSub) m_subLangComboBox->setCurrentIndex(i);
    }
    contentLayout->addWidget(m_subLangComboBox);

    QLabel *cloudSectionLabel = new QLabel("CLOUD & STORAGE", contentWidget);
    cloudSectionLabel->setStyleSheet(headerStyle);
    contentLayout->addWidget(cloudSectionLabel);

    QLabel *rcloneLabel = new QLabel("Rclone Config Path", contentWidget);
    rcloneLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(rcloneLabel);

    QHBoxLayout *rcloneLayout = new QHBoxLayout();
    rcloneLayout->setContentsMargins(0, 0, 0, 0);
    rcloneLayout->setSpacing(10);

    m_rclonePathEntry = new QLineEdit(contentWidget);
    #ifdef Q_OS_WIN
        QString defaultConfig = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/rclone/rclone.conf";
    #else
        QString defaultConfig = QDir::homePath() + "/.config/rclone/rclone.conf";
    #endif
    m_rclonePathEntry->setText(settings.value("rclone_conf_path", defaultConfig).toString());
    m_rclonePathEntry->setFixedHeight(42);
    m_rclonePathEntry->setStyleSheet(inputStyle);
    rcloneLayout->addWidget(m_rclonePathEntry, 1);

    QPushButton *browseBtn = new QPushButton("Browse", contentWidget);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setFixedHeight(42);
    browseBtn->setStyleSheet(R"(
        QPushButton { 
            background: #3d59a1; 
            color: white; 
            border-radius: 8px; 
            padding: 0 20px; 
            font-weight: bold; 
            border: none;
        }
        QPushButton:hover { background: #7aa2f7; color: #1a1b26; }
    )");
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Rclone Config", QDir::homePath(), "All Files (*)");
        if (!path.isEmpty()) m_rclonePathEntry->setText(path);
    });
    rcloneLayout->addWidget(browseBtn);
    contentLayout->addLayout(rcloneLayout);

    QLabel *rcloneFlagsLabel = new QLabel("Custom Rclone Flags (Optional)", contentWidget);
    rcloneFlagsLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(rcloneFlagsLabel);

    m_rcloneFlagsEntry = new QLineEdit(contentWidget);
    m_rcloneFlagsEntry->setPlaceholderText("e.g. --vfs-cache-mode=full --buffer-size=64M");
    m_rcloneFlagsEntry->setFixedHeight(42);
    m_rcloneFlagsEntry->setStyleSheet(inputStyle);
    m_rcloneFlagsEntry->setText(settings.value("rclone_flags", "").toString());
    contentLayout->addWidget(m_rcloneFlagsEntry);

    QLabel *metaSectionLabel = new QLabel("METADATA & CACHE", contentWidget);
    metaSectionLabel->setStyleSheet(headerStyle);
    contentLayout->addWidget(metaSectionLabel);

    QLabel *tmdbLabel = new QLabel("TMDB API Key (Required for Posters)", contentWidget);
    tmdbLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(tmdbLabel);

    m_tmdbEntry = new QLineEdit(contentWidget);
    m_tmdbEntry->setPlaceholderText("Enter your TMDB API Key...");
    m_tmdbEntry->setFixedHeight(42);
    m_tmdbEntry->setStyleSheet(inputStyle);
    m_tmdbEntry->setText(settings.value("tmdb_api_key", "").toString());
    contentLayout->addWidget(m_tmdbEntry);

    QLabel *metaLangLabel = new QLabel("Metadata Language", contentWidget);
    metaLangLabel->setStyleSheet(labelStyle);
    contentLayout->addWidget(metaLangLabel);

    m_metaLangEntry = new QLineEdit(contentWidget);
    m_metaLangEntry->setPlaceholderText("e.g. en-US");
    m_metaLangEntry->setFixedHeight(42);
    m_metaLangEntry->setStyleSheet(inputStyle);
    m_metaLangEntry->setText(settings.value("meta_lang", "en-US").toString());
    contentLayout->addWidget(m_metaLangEntry);

    m_clearCacheBtn = new QPushButton("Clear Image & Poster Cache", contentWidget);
    m_clearCacheBtn->setCursor(Qt::PointingHandCursor);
    m_clearCacheBtn->setFixedHeight(42);
    m_clearCacheBtn->setStyleSheet(R"(
        QPushButton { 
            background: #f7768e; 
            color: #1a1b26; 
            border-radius: 8px; 
            font-weight: bold; 
            font-size: 14px; 
            border: none;
        }
        QPushButton:hover { background: #ff98a4; }
    )");
    connect(m_clearCacheBtn, &QPushButton::clicked, this, &SettingsPage::clearImageCache);
    contentLayout->addWidget(m_clearCacheBtn);

    contentLayout->addSpacing(20);

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

    QLabel *footer = new QLabel("Changes to GPU or Cache settings require a player restart.\nPosters will update on next library rescan.", contentWidget);
    footer->setStyleSheet("color: #565f89; font-style: italic; margin-top: 10px;");
    contentLayout->addWidget(footer);

    scrollLayout->addWidget(contentWidget);
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea, 1);
}

void SettingsPage::saveSettings() {
    QSettings settings("Kino", "AppConfig");
    
    settings.setValue("gpu_accel", m_gpuToggle->isChecked());
    settings.setValue("tmdb_api_key", m_tmdbEntry->text().trimmed());
    settings.setValue("meta_lang", m_metaLangEntry->text().trimmed());
    settings.setValue("rclone_conf_path", m_rclonePathEntry->text().trimmed());
    settings.setValue("rclone_flags", m_rcloneFlagsEntry->text().trimmed());
    settings.setValue("mpv_cache_mb", m_mpvCacheSpinBox->value());
    settings.setValue("audio_lang", m_audioLangComboBox->currentData().toString());
    settings.setValue("sub_lang", m_subLangComboBox->currentData().toString());
    
    m_toast->showMessage("Settings saved successfully!", 2500);
    
    emit tmdbKeyUpdated();
}

void SettingsPage::clearImageCache() {
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/tmdb_images";
    QDir dir(cachePath);

    if (dir.exists()) {
        dir.removeRecursively(); 
        dir.mkpath(cachePath);   
        m_toast->showMessage("Image cache cleared successfully!", 2500);
    } else {
        m_toast->showMessage("Cache is already empty.", 2500);
    }
}