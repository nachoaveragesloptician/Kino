#include "playerpage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QStyle>
#include <QImage>
#include <QPixmap>
#include <QFontMetrics>
#include <QSettings>
#include <QCryptographicHash>
#include <QDebug>
#include <QStringList>
#include <QCoreApplication>
PlayerPage::~PlayerPage() {
    stop();
}

void PlayerPage::saveProgress() {
    if (m_duration > 0.0 && !m_currentFilePath.isEmpty()) {
        double pos = m_player->getProperty("time-pos").toDouble();
        if (pos > m_duration * 0.95) pos = 0.0;
        QString aid = m_player->getProperty("aid").toString();
        QString sid = m_player->getProperty("sid").toString();
        
        QSettings settings("Kino", "Player");
        QString hash = QString(QCryptographicHash::hash(m_currentFilePath.toUtf8(), QCryptographicHash::Md5).toHex());
        settings.beginGroup(hash);
        settings.setValue("position", (int)pos);
        settings.setValue("aid", aid);
        settings.setValue("sid", sid);
        settings.endGroup();
        
        settings.sync(); 
    }
}

TrackPopup::TrackPopup(QWidget *parent) : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *container = new QWidget(this);
    container->setObjectName("popupContainer");
    container->setAttribute(Qt::WA_StyledBackground, true); 
    container->setStyleSheet(R"(
        QWidget#popupContainer {
            background-color: #1a1b26;
            border: 1px solid #292e42;
            border-radius: 12px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(10, 15, 10, 15);
    layout->setSpacing(5);

    m_offBtn = new QPushButton("   OFF", container);
    m_offBtn->setCursor(Qt::PointingHandCursor);
    m_offBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #c0caf5;
            font-size: 14px;
            font-weight: bold;
            text-align: left;
            padding: 8px 12px;
            border-radius: 6px;
        }
        QPushButton:hover { background-color: #24283b; color: #f7768e; }
    )");
    layout->addWidget(m_offBtn);

    QLabel *subHeader = new QLabel("Embedded", container);
    subHeader->setObjectName("subHeaderLabel");
    subHeader->setStyleSheet("color: #565f89; font-size: 11px; font-weight: bold; padding-left: 8px; margin-top: 10px; margin-bottom: 5px;");
    layout->addWidget(subHeader);

    m_list = new QListWidget(container);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setStyleSheet(R"(
        QListWidget {
            background: transparent;
            outline: none;
        }
        QListWidget::item {
            color: #a9b1d6;
            font-size: 13px;
            padding: 10px 12px;
            border-radius: 6px;
            margin-bottom: 4px;
        }
        QListWidget::item:hover {
            background-color: #24283b;
            color: #c0caf5;
        }
        QListWidget::item:selected {
            background-color: #7aa2f7;
            color: #1a1b26;
            font-weight: bold;
        }
    )");
    layout->addWidget(m_list);

    mainLayout->addWidget(container);

    connect(m_offBtn, &QPushButton::clicked, [this]() {
        emit trackDisabled();
        close(); 
    });

    connect(m_list, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        int id = item->data(Qt::UserRole).toInt();
        emit trackSelected(id);
        close();
    });
}

void TrackPopup::populate(const QList<QPair<int, QString>> &tracks, const QString &type, const QString &currentId) {
    m_list->clear();
    
    QLabel *header = this->findChild<QLabel*>("subHeaderLabel");
    if (header) header->setText("Embedded"); 

    bool isOff = (currentId == "no" || currentId == "false" || currentId.isEmpty());
    if (isOff) {
        m_offBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #7aa2f7;
                color: #1a1b26;
                font-size: 14px;
                font-weight: bold;
                text-align: left;
                padding: 8px 12px;
                border-radius: 6px;
            }
        )");
    } else {
        m_offBtn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                color: #c0caf5;
                font-size: 14px;
                font-weight: bold;
                text-align: left;
                padding: 8px 12px;
                border-radius: 6px;
            }
            QPushButton:hover { background-color: #24283b; color: #f7768e; }
        )");
    }

    QFontMetrics fm(m_list->font());
    int maxWidth = 260;
    
    for (const auto &track : tracks) {
        QListWidgetItem *item = new QListWidgetItem(track.second);
        item->setData(Qt::UserRole, track.first); 
        m_list->addItem(item);
        if (QString::number(track.first) == currentId) {
            item->setSelected(true);
        }
        
        int textWidth = fm.horizontalAdvance(track.second);
        if (textWidth + 60 > maxWidth) {
            maxWidth = textWidth + 60;
        }
    }
    
    if (maxWidth > 800) maxWidth = 800; 
    
    int targetHeight = (tracks.size() * 40) + 120;
    if (targetHeight > 600) targetHeight = 600; 
    
    this->setFixedSize(maxWidth, targetHeight);
}


PlayerPage::PlayerPage(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("PlayerPage { background-color: #000000; }");

    m_player = new MpvWidget(this);
    
    setupLoader();
    setupOverlay();

    m_player->installEventFilter(this);
    m_player->setMouseTracking(true);
    
    m_overlay->installEventFilter(this);
    m_overlay->setMouseTracking(true);
    
    m_progressSlider->installEventFilter(this); 
    m_verticalVolumeSlider->installEventFilter(this); 

    connect(m_player, &MpvWidget::positionChanged, this, [this](double pos){
        if (!m_progressSlider->isSliderDown()) {
            m_progressSlider->setValue(pos);
            m_timeLabel->setText(formatTime(pos) + " / " + formatTime(m_duration));
        }
        if (pos > 0.5 && m_loaderWidget->isVisible()) {
            m_loaderWidget->hide();
            showControls();
        }
    });

    connect(m_player, &MpvWidget::durationChanged, this, [this](double dur){ 
        m_duration = dur; 
        m_progressSlider->setRange(0, dur); 
    });

    connect(m_player, &MpvWidget::volumeChanged, this, [this](double vol){
        m_volume = vol;
        if (!m_verticalVolumeSlider->isSliderDown()) {
            m_verticalVolumeSlider->setValue(vol);
            m_volumeLabel->setText(QString::number(static_cast<int>(vol)));
        }
        updateVolumeIcon();
    });

    connect(m_player, &MpvWidget::muteChanged, this, [this](bool muted){
        m_isMuted = muted;
        updateVolumeIcon();
    });

    connect(m_progressSlider, &QSlider::sliderReleased, this, [this](){
        m_player->command(QVariantList() << "seek" << m_progressSlider->value() << "absolute");
    });
    
    connect(m_verticalVolumeSlider, &QSlider::valueChanged, this, [this](int val){
        m_player->setProperty("volume", val);
        if (m_isMuted && val > 0) m_player->setProperty("mute", "no"); 
        m_volumeLabel->setText(QString::number(val));
    });

    m_hideTimer = new QTimer(this); 
    m_hideTimer->setInterval(3000); 
    connect(m_hideTimer, &QTimer::timeout, m_overlay, &QWidget::hide);
    
    m_volHideTimer = new QTimer(this); 
    m_volHideTimer->setInterval(300); 
    m_volHideTimer->setSingleShot(true); 
    connect(m_volHideTimer, &QTimer::timeout, m_volumePopup, &QWidget::hide);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &PlayerPage::stop);
}

void PlayerPage::setupLoader() {
    m_loaderWidget = new QWidget(m_player);
    m_loaderWidget->setObjectName("loaderWidget");
    m_loaderWidget->setAttribute(Qt::WA_StyledBackground, true); 
    m_loaderWidget->setStyleSheet("QWidget#loaderWidget { background: #1a1b26; }");

    m_bannerLabel = new QLabel(m_loaderWidget);
    m_bannerLabel->setScaledContents(true);
    m_bannerLabel->setStyleSheet("background: transparent; border: none;");
    
    m_loaderDarkOverlay = new QWidget(m_loaderWidget);
    m_loaderDarkOverlay->setAttribute(Qt::WA_StyledBackground, true); 
    m_loaderDarkOverlay->setStyleSheet("background: rgba(26, 27, 38, 0.75); border: none;"); 

    QVBoxLayout *lay = new QVBoxLayout(m_loaderWidget);
    lay->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setContentsMargins(35, 25, 35, 15);
    QPushButton *backBtn = new QPushButton("❮", m_loaderWidget);
    backBtn->setFixedSize(45, 45);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.05); color: white; border-radius: 22px; font-weight: bold; font-size: 20px; padding-bottom: 3px; padding-right: 2px; border: none; } QPushButton:hover { background: #f7768e; color: #1a1b26; }");
    connect(backBtn, &QPushButton::clicked, this, [this](){ stop(); emit backRequested(); });
    
    topBar->addWidget(backBtn);
    topBar->addStretch();
    lay->addLayout(topBar);

    lay->addStretch();

    m_spinnerLabel = new QLabel("", m_loaderWidget);
    m_spinnerLabel->setStyleSheet("color: #ffffff; font-size: 48px; font-weight: bold; font-family: 'Segoe UI', sans-serif; background: transparent; border: none;");
    m_spinnerLabel->setAlignment(Qt::AlignCenter);
    m_spinnerLabel->setWordWrap(true);
    lay->addWidget(m_spinnerLabel);

    m_loadingSubText = new QLabel("Loading stream...", m_loaderWidget);
    m_loadingSubText->setStyleSheet("color: #7aa2f7; font-size: 18px; font-weight: bold; font-family: 'Segoe UI', sans-serif; background: transparent; border: none;");
    m_loadingSubText->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_loadingSubText);

    lay->addStretch();
    m_loaderWidget->hide();
}

void PlayerPage::setupOverlay() {
    m_overlay = new QWidget(m_player); 
    m_overlay->setAttribute(Qt::WA_StyledBackground, true);
    m_overlay->setStyleSheet("background: transparent;");
    
    QVBoxLayout *overlayLayout = new QVBoxLayout(m_overlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0); 

    QWidget *topPanel = new QWidget(m_overlay);
    topPanel->setAttribute(Qt::WA_StyledBackground, true);
    topPanel->setStyleSheet("background: transparent; border: none;");
    
    QHBoxLayout *topBar = new QHBoxLayout(topPanel);
    topBar->setContentsMargins(35, 25, 35, 40); 
    topBar->setAlignment(Qt::AlignVCenter);
    
    QPushButton *backBtn = new QPushButton("❮", topPanel);
    backBtn->setFixedSize(45, 45); 
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.05); color: white; border-radius: 22px; font-size: 20px; font-weight: bold; padding-bottom: 3px; padding-right: 2px; border: none;} QPushButton:hover { background: #f7768e; color: #1a1b26; }");
    connect(backBtn, &QPushButton::clicked, this, [this](){ stop(); emit backRequested(); });
    topBar->addWidget(backBtn);

    m_titleLabel = new QLabel("", topPanel);
    m_titleLabel->setStyleSheet("color: #c0caf5; font-size: 24px; font-weight: bold; margin-left: 15px; font-family: 'Segoe UI'; background: transparent; border: none;");
    topBar->addWidget(m_titleLabel, 1);
    overlayLayout->addWidget(topPanel);
    
    overlayLayout->addStretch();

    QWidget *bottomPanel = new QWidget(m_overlay);
    bottomPanel->setAttribute(Qt::WA_StyledBackground, true);
    bottomPanel->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(26,27,38,0.0), stop:1 rgba(26,27,38,0.95)); border: none;");
    
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(35, 20, 35, 25);

    m_progressSlider = new QSlider(Qt::Horizontal, bottomPanel);
    m_progressSlider->setCursor(Qt::PointingHandCursor);
    m_progressSlider->setStyleSheet(R"(
        QSlider { background: transparent; height: 30px; }
        QSlider::groove:horizontal { height: 6px; border-radius: 3px; background: rgba(255, 255, 255, 0.15); } 
        QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3d59a1, stop:1 #7aa2f7); border-radius: 3px; } 
        QSlider::handle:horizontal { background: white; width: 14px; height: 14px; margin: -4px 0; border-radius: 7px; }
        QSlider::handle:horizontal:hover { background: #f7768e; width: 18px; margin: -6px -2px; border-radius: 9px; }
    )");
    bottomLayout->addWidget(m_progressSlider);

    QHBoxLayout *ctrlRow = new QHBoxLayout();
    ctrlRow->setSpacing(25);
    ctrlRow->setAlignment(Qt::AlignVCenter);

    m_playPauseBtn = new QPushButton(bottomPanel);
    m_playPauseBtn->setFixedSize(50, 50); 
    m_playPauseBtn->setCursor(Qt::PointingHandCursor);
    m_playPauseBtn->setStyleSheet("background: transparent; border: none;");
    m_playPauseBtn->setIcon(createWhiteIcon(":/icons/pause.svg")); 
    m_playPauseBtn->setIconSize(QSize(32, 32));
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PlayerPage::togglePlayPause);
    ctrlRow->addWidget(m_playPauseBtn);
    
    m_volumeBtn = new QPushButton(bottomPanel);
    m_volumeBtn->setFixedSize(45, 45); 
    m_volumeBtn->setCursor(Qt::PointingHandCursor);
    m_volumeBtn->setStyleSheet("background: transparent; border: none;");
    m_volumeBtn->setIconSize(QSize(28, 28));
    m_volumeBtn->installEventFilter(this);
    updateVolumeIcon();
    connect(m_volumeBtn, &QPushButton::clicked, this, [this](){ m_player->setProperty("mute", !m_isMuted ? "yes" : "no"); });
    ctrlRow->addWidget(m_volumeBtn);

    m_timeLabel = new QLabel("00:00 / 00:00", bottomPanel);
    m_timeLabel->setStyleSheet("color: #c0caf5; font-size: 14px; font-family: monospace; font-weight: bold; background: transparent; border: none;");
    ctrlRow->addWidget(m_timeLabel);

    ctrlRow->addStretch();

    m_audioBtn = new QPushButton(bottomPanel);
    m_audioBtn->setFixedSize(45, 45); 
    m_audioBtn->setCursor(Qt::PointingHandCursor);
    m_audioBtn->setIcon(createWhiteIcon(":/icons/tracks.svg"));
    m_audioBtn->setIconSize(QSize(26, 26));
    m_audioBtn->setStyleSheet("background: transparent; border: none;");
    connect(m_audioBtn, &QPushButton::clicked, this, [this](){ showCustomMenu("audio"); });
    ctrlRow->addWidget(m_audioBtn);

    m_subBtn = new QPushButton(bottomPanel);
    m_subBtn->setFixedSize(45, 45); 
    m_subBtn->setCursor(Qt::PointingHandCursor);
    m_subBtn->setIcon(createWhiteIcon(":/icons/subtitles.svg"));
    m_subBtn->setIconSize(QSize(26, 26));
    m_subBtn->setStyleSheet("background: transparent; border: none;");
    connect(m_subBtn, &QPushButton::clicked, this, [this](){ showCustomMenu("sub"); });
    ctrlRow->addWidget(m_subBtn);

    bottomLayout->addLayout(ctrlRow);
    overlayLayout->addWidget(bottomPanel);

    m_volumePopup = new QWidget(m_player);
    m_volumePopup->setFixedSize(50, 160);
    m_volumePopup->setAttribute(Qt::WA_StyledBackground, true);
    m_volumePopup->setStyleSheet("background: rgba(30, 30, 46, 0.95); border-radius: 12px; border: 1px solid #414868;");
    m_volumePopup->installEventFilter(this);
    
    QVBoxLayout *volLay = new QVBoxLayout(m_volumePopup);
    m_volumeLabel = new QLabel("100", m_volumePopup);
    m_volumeLabel->setStyleSheet("color: white; font-size: 12px; font-weight: bold; background: transparent; border: none;");
    m_volumeLabel->setAlignment(Qt::AlignCenter);
    volLay->addWidget(m_volumeLabel);
    
    m_verticalVolumeSlider = new QSlider(Qt::Vertical, m_volumePopup);
    m_verticalVolumeSlider->setRange(0, 100);
    m_verticalVolumeSlider->setStyleSheet("QSlider::groove:vertical { width: 6px; background: rgba(255,255,255,0.2); border-radius: 3px; } QSlider::sub-page:vertical { background: #7aa2f7; border-radius: 3px; } QSlider::handle:vertical { background: white; height: 14px; margin: 0 -4px; border-radius: 7px; }");
    volLay->addWidget(m_verticalVolumeSlider);
    
    m_volumePopup->hide();
}

void PlayerPage::resizeEvent(QResizeEvent *event) { 
    QWidget::resizeEvent(event); 
    m_player->setGeometry(rect()); 
    m_overlay->setGeometry(m_player->rect()); 
    m_loaderWidget->setGeometry(m_player->rect());
    
    if(m_bannerLabel) m_bannerLabel->setGeometry(m_loaderWidget->rect());
    if(m_loaderDarkOverlay) m_loaderDarkOverlay->setGeometry(m_loaderWidget->rect());
}

void PlayerPage::play(const QString &filePath, const QString &title, const QPixmap &banner) {
    m_spinnerLabel->setText(title); 
    
    if (!banner.isNull()) {
        m_bannerLabel->setPixmap(banner);
        m_bannerLabel->show();
        m_loaderDarkOverlay->show();
        m_loaderWidget->setStyleSheet("QWidget#loaderWidget { background: transparent; }");
    } else {
        m_bannerLabel->hide();
        m_loaderDarkOverlay->hide();
        m_loaderWidget->setStyleSheet("QWidget#loaderWidget { background: #1a1b26; }");
    }

    m_loaderWidget->show();
    m_loaderWidget->raise(); 
    m_overlay->hide(); 
    
    m_isPlaying = true;
    m_playPauseBtn->setIcon(createWhiteIcon(":/icons/pause.svg")); 
    m_currentFilePath = filePath;
    m_titleLabel->setText(title);

    QTimer::singleShot(100, this, [this, filePath]() {
        QSettings settings("Kino", "Player");
        QString hash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
        settings.beginGroup(hash);
        int startPos = settings.value("position", 0).toInt();
        QString aid = settings.value("aid", "auto").toString();
        QString sid = settings.value("sid", "auto").toString();
        settings.endGroup();

        QStringList opts;
        if (startPos > 0) opts << QString("start=%1").arg(startPos);
        if (aid != "auto" && !aid.isEmpty()) opts << QString("aid=%1").arg(aid);
        if (sid != "auto" && !sid.isEmpty()) opts << QString("sid=%1").arg(sid);

        QVariantList args;
        if (opts.isEmpty()) {
            args << "loadfile" << filePath << "replace";
        } else {
            args << "loadfile" << filePath << "replace" << "0" << opts.join(",");
        }

        m_player->setProperty("pause", "no");
        m_player->command(args);
    });
}

void PlayerPage::stop() { 
    if (m_duration > 0.0 && !m_currentFilePath.isEmpty()) {
        double pos = m_player->getProperty("time-pos").toDouble();
        if (pos > m_duration * 0.95) pos = 0.0;
        
        QString aid = m_player->getProperty("aid").toString();
        QString sid = m_player->getProperty("sid").toString();
        QSettings settings("Kino", "Player");
        QString hash = QString(QCryptographicHash::hash(m_currentFilePath.toUtf8(), QCryptographicHash::Md5).toHex());
        settings.beginGroup(hash);
        settings.setValue("position", (int)pos);
        settings.setValue("aid", aid);
        settings.setValue("sid", sid);
        settings.endGroup();
        
        settings.sync();
        
        m_currentFilePath.clear(); 
    }
    m_player->command(QVariantList() << "stop"); 
    m_loaderWidget->hide(); 
}

void PlayerPage::togglePlayPause() {
    m_isPlaying = !m_isPlaying;
    m_playPauseBtn->setIcon(createWhiteIcon(m_isPlaying ? ":/icons/pause.svg" : ":/icons/play.svg"));
    m_player->setProperty("pause", m_isPlaying ? "no" : "yes");
}

void PlayerPage::showControls() { 
    m_overlay->raise();
    if (!m_overlay->isVisible()) {
        m_overlay->show(); 
    }
    m_hideTimer->start(); 
}

QString PlayerPage::formatTime(double seconds) {
    int h = seconds / 3600;
    int m = (int(seconds) % 3600) / 60;
    int s = int(seconds) % 60;
    if (h > 0) return QString::asprintf("%02d:%02d:%02d", h, m, s);
    return QString::asprintf("%02d:%02d", m, s);
}

void PlayerPage::updateVolumeIcon() {
    if (m_isMuted || m_volume <= 0) m_volumeBtn->setIcon(createWhiteIcon(":/icons/volume-mute.svg"));
    else if (m_volume < 33) m_volumeBtn->setIcon(createWhiteIcon(":/icons/volume-low.svg"));
    else if (m_volume < 66) m_volumeBtn->setIcon(createWhiteIcon(":/icons/volume-mid.svg"));
    else m_volumeBtn->setIcon(createWhiteIcon(":/icons/volume-high.svg"));
}

QIcon PlayerPage::createWhiteIcon(const QString &path) {
    QImage img(path);
    if (!img.isNull()) { img.invertPixels(QImage::InvertRgb); return QIcon(QPixmap::fromImage(img)); }
    return QIcon();
}

QList<QPair<int, QString>> PlayerPage::getTrackList(const QString &type) {
    QList<QPair<int, QString>> tracks;
    
    int count = m_player->getProperty("track-list/count").toInt();
    for (int i = 0; i < count; ++i) {
        QString trackType = m_player->getProperty(QString("track-list/%1/type").arg(i)).toString();
        
        if (trackType == type) {
            int id = m_player->getProperty(QString("track-list/%1/id").arg(i)).toInt();
            QString lang = m_player->getProperty(QString("track-list/%1/lang").arg(i)).toString();
            QString title = m_player->getProperty(QString("track-list/%1/title").arg(i)).toString();
            
            QString displayName;
            if (!title.isEmpty() && !lang.isEmpty()) displayName = QString("%1 [%2]").arg(title, lang.toUpper());
            else if (!title.isEmpty()) displayName = title;
            else if (!lang.isEmpty()) displayName = QString("Track %1 [%2]").arg(id).arg(lang.toUpper());
            else displayName = QString("Track %1").arg(id);
            
            tracks.append({id, displayName});
        }
    }
    return tracks;
}

void PlayerPage::showCustomMenu(const QString &type) {
    QList<QPair<int, QString>> tracks = getTrackList(type);
    if (tracks.isEmpty()) return;

    QString currentProp = (type == "sub") ? "sid" : "aid";
    QString currentId = m_player->getProperty(currentProp).toString();

    TrackPopup *popup = new TrackPopup(this);
    popup->populate(tracks, type, currentId);

    QPushButton *anchorBtn = (type == "sub") ? m_subBtn : m_audioBtn;
    QPoint globalBtnPos = anchorBtn->mapToGlobal(QPoint(0, 0));

    int xPos = globalBtnPos.x() - (popup->width() / 2) + (anchorBtn->width() / 2);
    int yPos = globalBtnPos.y() - popup->height() - 10;
    
    popup->move(xPos, yPos);

    connect(popup, &TrackPopup::trackSelected, this, [this, type](int id) {
        m_player->setProperty(type == "sub" ? "sid" : "aid", id);
    });

    connect(popup, &TrackPopup::trackDisabled, this, [this, type]() {
        m_player->setProperty(type == "sub" ? "sid" : "aid", "no");
    });

    popup->show();
}

void PlayerPage::showVolumePopupTemp() {
    m_volumePopup->move(m_volumeBtn->mapTo(m_player, QPoint(-5, -155)));
    m_volumePopup->show();
    m_volHideTimer->start();
}

void PlayerPage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        togglePlayPause();
    } else if (event->key() == Qt::Key_Up) { 
        m_player->setProperty("volume", qMin(100.0, m_volume + 5.0)); 
        showVolumePopupTemp(); 
    } else if (event->key() == Qt::Key_Down) { 
        m_player->setProperty("volume", qMax(0.0, m_volume - 5.0)); 
        showVolumePopupTemp(); 
    } else if (event->key() == Qt::Key_Left) {
        m_player->command(QVariantList() << "seek" << -10); 
        showControls();
    } else if (event->key() == Qt::Key_Right) {
        m_player->command(QVariantList() << "seek" << 10); 
        showControls();
    }
    event->accept();
}

bool PlayerPage::eventFilter(QObject *obj, QEvent *event) {
    if ((obj == m_player || obj == m_overlay) && event->type() == QEvent::MouseMove) {
        showControls();
    }
    
    if (obj == m_volumeBtn || obj == m_volumePopup) {
        if (event->type() == QEvent::Enter) {
            m_volHideTimer->stop(); 
            if (obj == m_volumeBtn) showVolumePopupTemp();
        } else if (event->type() == QEvent::Leave) {
            m_volHideTimer->start(500); 
        }
    }
    
    if (obj == m_progressSlider && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            int val = QStyle::sliderValueFromPosition(m_progressSlider->minimum(), 
                                                      m_progressSlider->maximum(), 
                                                      mouseEvent->pos().x(), 
                                                      m_progressSlider->width());
            m_progressSlider->setValue(val);
            m_player->command(QVariantList() << "seek" << val << "absolute");
        }
    }
    
    return QWidget::eventFilter(obj, event);
}