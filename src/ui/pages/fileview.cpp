#include "fileview.h"
#include "mediadelegate.h"
#include "mediaparser.h" 
#include "tmdbclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScroller>
#include <QStandardItem>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QDate>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QListWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCoreApplication>
FileView::FileView(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); 
    mainLayout->setSpacing(0);

    m_mainContentWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(m_mainContentWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/Kino";
    QDir().mkpath(configDir);
    m_cacheFilePath = configDir + "/library_cache.json";
    m_mediaInfoCachePath = configDir + "/mediainfo_cache.json";
    
    QTimer::singleShot(50, this, [this]() {
        loadMediaInfoCache();
    });

    m_probeTimer = new QTimer(this);
    m_probeTimer->setSingleShot(true);
    connect(m_probeTimer, &QTimer::timeout, this, &FileView::executeMediaInfoProbe);

    m_toast = new Toast(this);
    m_tmdb = new TmdbClient(this);
    connect(m_tmdb, &TmdbClient::posterLoaded, this, &FileView::onPosterLoaded);
    connect(m_tmdb, &TmdbClient::backdropLoaded, this, &FileView::onBackdropLoaded);
    connect(m_tmdb, &TmdbClient::metadataLoaded, this, &FileView::onMetadataLoaded);

    QWidget *headerContainer = new QWidget(m_mainContentWidget);
    headerContainer->setFixedHeight(100);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(40, 40, 40, 20);
    headerLayout->setSpacing(20);
    headerLayout->setAlignment(Qt::AlignVCenter);

    m_backBtn = new QPushButton("❮", this);
    m_backBtn->setFixedSize(40, 40); 
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.08); color: white; border-radius: 20px; font-weight: bold; font-size: 18px; padding-right: 2px; border: 1px solid rgba(255,255,255,0.1); } QPushButton:hover { background: #7aa2f7; color: #1a1b26; border: none; }");
    m_backBtn->hide();
    connect(m_backBtn, &QPushButton::clicked, this, &FileView::onBackClicked);
    headerLayout->addWidget(m_backBtn);

    m_titleLabel = new QLabel("Library", this);
    m_titleLabel->setStyleSheet("color: #7aa2f7; font-size: 32px; font-weight: 800; background: transparent; border: none;");
    headerLayout->addWidget(m_titleLabel);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Search collection...");
    m_searchBar->setFixedHeight(40); 
    
    QPixmap searchPix(":/icons/search.svg");
    QPainter painter(&searchPix);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(searchPix.rect(), QColor("#9aa5ce")); 
    painter.end();
    QAction *searchIcon = new QAction(m_searchBar);
    searchIcon->setIcon(QIcon(searchPix));
    m_searchBar->addAction(searchIcon, QLineEdit::LeadingPosition);

    m_searchBar->setStyleSheet(R"(
        QLineEdit { background: #16161e; color: #c0caf5; border: 1px solid #2f3549; border-radius: 20px; padding-left: 10px; font-size: 14px; font-weight: 500; }
        QLineEdit::placeholder { color: #565f89; }
        QLineEdit:focus { border: 1px solid #7aa2f7; background: #1f2335; }
    )");
    headerLayout->addWidget(m_searchBar, 1);

    m_rescanBtn = new QPushButton("Rescan", this);
    m_rescanBtn->setCursor(Qt::PointingHandCursor);
    m_rescanBtn->setFixedHeight(40);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(m_rescanBtn);
    shadow->setBlurRadius(15); shadow->setColor(QColor(0, 0, 0, 80)); shadow->setOffset(0, 4);
    m_rescanBtn->setGraphicsEffect(shadow);
    m_rescanBtn->setStyleSheet(R"(
        QPushButton { background: #7aa2f7; color: #1a1b26; border-radius: 20px; padding: 0 30px; font-weight: 700; font-size: 14px; border: none; outline: none; }
        QPushButton:hover { background: #89b4fa; border-radius: 20px; }
        QPushButton:pressed { background: #6b93e8; border-radius: 20px; border: none; outline: none; }
    )");
    connect(m_rescanBtn, &QPushButton::clicked, this, &FileView::showRescanMenu);
    headerLayout->addWidget(m_rescanBtn);
    leftLayout->addWidget(headerContainer);

    m_model = new QStandardItemModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(m_searchBar, &QLineEdit::textChanged, m_proxy, &QSortFilterProxyModel::setFilterFixedString);

    m_view = new QListView(this);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->verticalScrollBar()->setSingleStep(20);
    m_view->setModel(m_proxy);
    m_view->setItemDelegate(new MediaDelegate(this));
    m_view->setViewMode(QListView::IconMode);
    m_view->setResizeMode(QListView::Adjust);
    m_view->setMovement(QListView::Static);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setSpacing(15); 
    
    m_view->setStyleSheet(R"(
        QListView { background: transparent; outline: none; border: none; padding: 0px 40px; }
        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
        QScrollBar:handle:vertical { background: #414868; min-height: 40px; border-radius: 4px; }
        QScrollBar:handle:vertical:hover { background: #7aa2f7; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");
    
    QScroller::grabGesture(m_view->viewport(), QScroller::LeftMouseButtonGesture);
    connect(m_view, &QListView::clicked, this, &FileView::onItemClicked);
    m_view->viewport()->installEventFilter(this);
    m_view->setMouseTracking(true);
    m_view->viewport()->setAttribute(Qt::WA_Hover);
    leftLayout->addWidget(m_view, 1);

    m_emptyWidget = new QWidget(this);
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    m_emptyIconLabel = new QLabel(m_emptyWidget);
    QPixmap emptyPix = QIcon(":/icons/empty.svg").pixmap(80, 80);
    if (!emptyPix.isNull()) {
        QPainter painter(&emptyPix);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(emptyPix.rect(), QColor("#3b4261")); 
        painter.end();
    }

m_emptyIconLabel->setPixmap(emptyPix);
    m_emptyIconLabel->setStyleSheet("font-size: 80px; color: #3b4261; background: transparent;");
    m_emptyIconLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(m_emptyIconLabel);
    m_emptyTextLabel = new QLabel("No drives mounted.\nGo to the Drives tab to mount one!", m_emptyWidget);
    m_emptyTextLabel->setStyleSheet("color: #565f89; font-size: 18px; font-weight: 600; line-height: 1.5;");
    m_emptyTextLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(m_emptyTextLabel);
    leftLayout->addWidget(m_emptyWidget, 1);

    m_view->hide();
    m_emptyWidget->show();

    setupSidebar();

    mainLayout->addWidget(m_mainContentWidget, 1);
    mainLayout->addWidget(m_sidebarWidget);
}

FileView::~FileView() { 
    stopScan(); 
    for (auto proc : findChildren<QProcess*>()) {
        if (proc->state() == QProcess::Running) {
            proc->kill();
            proc->waitForFinished(1000); 
        }
    }
}

void FileView::setupSidebar() {
    m_sidebarWidget = new QWidget(this);
    m_sidebarWidget->setFixedWidth(420); 
    m_sidebarWidget->setStyleSheet("QWidget { background: #16161e; border-left: 1px solid #1f2335; }");
    
    QVBoxLayout *sideLayout = new QVBoxLayout(m_sidebarWidget);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(m_sidebarWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
        QScrollBar:handle:vertical { background: #414868; border-radius: 3px; }
        QScrollBar:handle:vertical:hover { background: #7aa2f7; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");
    
    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setFixedWidth(410); 

    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, 0, 20); 
    contentLayout->setSpacing(0);
    contentLayout->setAlignment(Qt::AlignTop);

    QWidget *posterContainer = new QWidget(scrollContent);
    posterContainer->setFixedSize(410, 615); 
    
    m_sidebarPoster = new QLabel(posterContainer);
    m_sidebarPoster->setGeometry(0, 0, 410, 615);
    m_sidebarPoster->setScaledContents(true);
    m_sidebarPoster->setStyleSheet("background: #1f2335;");

    QWidget *gradientOverlay = new QWidget(posterContainer);
    gradientOverlay->setGeometry(0, 0, 410, 615);
    gradientOverlay->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0.4 transparent, stop:0.8 rgba(22, 22, 30, 0.9), stop:1 rgba(22, 22, 30, 1));");

    QVBoxLayout *posterLayout = new QVBoxLayout(posterContainer);
    posterLayout->setContentsMargins(25, 0, 25, 15);
    posterLayout->addStretch();

    m_sidebarTitle = new QLabel("", posterContainer);
    m_sidebarTitle->setWordWrap(true); 
    m_sidebarTitle->setStyleSheet("color: #ffffff; font-size: 34px; font-weight: 900; background: transparent; border: none; padding: 0; margin: 0;"); 
    posterLayout->addWidget(m_sidebarTitle);

    contentLayout->addWidget(posterContainer);

    QWidget *textContainer = new QWidget(scrollContent);
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(25, 10, 25, 0); 
    textLayout->setSpacing(12);

    m_sidebarMeta = new QLabel("", textContainer);
    m_sidebarMeta->setStyleSheet("color: #7dcfff; font-size: 13px; font-weight: 600; border: none;");
    textLayout->addWidget(m_sidebarMeta);

    m_sidebarOverview = new QLabel("", textContainer);
    m_sidebarOverview->setWordWrap(true);
    m_sidebarOverview->setStyleSheet("color: #9aa5ce; font-size: 14px; line-height: 1.5; border: none;");
    textLayout->addWidget(m_sidebarOverview);

    m_sidebarFileInfo = new QLabel("", textContainer);
    m_sidebarFileInfo->setWordWrap(true);
    m_sidebarFileInfo->setStyleSheet("color: #c0caf5; background: #1f2335; padding: 15px; border-radius: 8px; font-size: 13px; line-height: 1.5; margin-top: 5px; border: 1px solid #2f3549;");
    m_sidebarFileInfo->hide();
    textLayout->addWidget(m_sidebarFileInfo);

    contentLayout->addWidget(textContainer);
    scrollArea->setWidget(scrollContent);
    sideLayout->addWidget(scrollArea, 1);

    QWidget *playContainer = new QWidget(m_sidebarWidget);
    playContainer->setStyleSheet("background: #16161e; border-top: 1px solid #1f2335; border-left: none;");
    QVBoxLayout *playLayout = new QVBoxLayout(playContainer);
    playLayout->setContentsMargins(25, 20, 25, 25);
    
    m_sidebarPlayBtn = new QPushButton("▶ Play Stream", playContainer);
    m_sidebarPlayBtn->setFixedHeight(50);
    m_sidebarPlayBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarPlayBtn->setStyleSheet(R"(
        QPushButton { background: #7aa2f7; color: #1a1b26; border-radius: 8px; font-weight: bold; font-size: 16px; border: none; }
        QPushButton:hover { background: #89b4fa; }
        QPushButton:disabled { background: #2f3549; color: #565f89; }
    )");
    m_sidebarPlayBtn->setEnabled(false);
    connect(m_sidebarPlayBtn, &QPushButton::clicked, this, &FileView::onPlayButtonClicked);
    playLayout->addWidget(m_sidebarPlayBtn);
    
    sideLayout->addWidget(playContainer);
}

void FileView::clearSidebar() {
    m_sidebarPoster->clear();
    m_sidebarTitle->clear();
    m_sidebarMeta->clear();
    m_sidebarOverview->clear();
    m_sidebarFileInfo->hide();
    m_sidebarPlayBtn->setEnabled(false);
}

void FileView::loadMediaInfoCache() {
    QFile file(m_mediaInfoCachePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        for (auto it = root.begin(); it != root.end(); ++it) {
            m_mediaInfoCache[it.key()] = it.value().toString();
        }
    }
}

void FileView::saveMediaInfoCache() {
    QJsonObject root;
    for (auto it = m_mediaInfoCache.begin(); it != m_mediaInfoCache.end(); ++it) {
        root[it.key()] = it.value();
    }
    QFile file(m_mediaInfoCachePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
    }
}

void FileView::requestMediaInfo(const QString &path, const QString &filename) {
    if (m_mediaInfoCache.contains(path)) {
        m_sidebarFileInfo->setText(m_mediaInfoCache[path]);
        m_sidebarFileInfo->show();
        return;
    }

    QString instantInfo = this->formatExtendedMediaInfo(filename);
    m_sidebarFileInfo->setText(instantInfo + "<br><br><span style='color: #e0af68; font-size: 11px;'>⏳ Probing stream for exact bitrates...</span>");
    m_sidebarFileInfo->show();

    m_pathToProbe = path;
    m_filenameToProbe = filename;
    m_probeTimer->start(400); 
}

void FileView::executeMediaInfoProbe() {
    QString path = m_pathToProbe;
    QString filename = m_filenameToProbe;

    QProcess *proc = new QProcess(this);
    #ifdef Q_OS_WIN
        QString execPath = QStandardPaths::findExecutable("mediainfo.exe");
        if (execPath.isEmpty()) execPath = "mediainfo.exe"; 
    #else
        QString execPath = QStandardPaths::findExecutable("mediainfo");
        if (execPath.isEmpty()) execPath = "mediainfo"; 
    #endif
    
    QTimer *timeoutTimer = new QTimer(proc);
    timeoutTimer->setSingleShot(true);

    connect(proc, &QProcess::errorOccurred, [this, path, filename, timeoutTimer, proc](QProcess::ProcessError) {
        timeoutTimer->stop();
        if (m_currentPlayPath == path) m_sidebarFileInfo->setText(this->formatExtendedMediaInfo(filename));
        proc->deleteLater();
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
        [this, path, filename, proc, timeoutTimer](int exitCode, QProcess::ExitStatus status) {
            timeoutTimer->stop();
            QString html;
            
            if (status == QProcess::NormalExit && exitCode == 0) {
                QByteArray output = proc->readAllStandardOutput();
                html = parseMediaInfoText(output, path, filename);
            }
            
            if (html.isEmpty()) {
                html = this->formatExtendedMediaInfo(filename);
            } else {
                m_mediaInfoCache[path] = html;
                saveMediaInfoCache();
            }

            if (m_currentPlayPath == path) {
                m_sidebarFileInfo->setText(html); 
            }
            proc->deleteLater();
    });

    connect(timeoutTimer, &QTimer::timeout, [this, path, filename, proc]() {
        if (proc->state() == QProcess::Running) {
            proc->kill(); 
            proc->waitForFinished(1000);
        }
        if (m_currentPlayPath == path) m_sidebarFileInfo->setText(this->formatExtendedMediaInfo(filename));
    });

    QString nativePath = QDir::toNativeSeparators(path);
    proc->start(execPath, QStringList() << nativePath);
    timeoutTimer->start(30000); 
}

QString FileView::parseMediaInfoText(const QByteArray &output, const QString &path, const QString &filename) {
    QStringList lines = QString::fromUtf8(output).split('\n');
    
    QStringList videoList, audioList, subList;
    QString overallBitrate = "Unknown";
    
    enum Section { None, General, Video, Audio, Text };
    Section currentSection = None;

    QString vFormat, vWidth, vHeight, vBitDepth, vHdr;
    QString aFormat, aChannels, aLang, aBitrate;

    auto pushVideo = [&]() {
        if (!vFormat.isEmpty()) {
            QString v = vFormat;
            if (!vWidth.isEmpty() && !vHeight.isEmpty()) v += " (" + vWidth + "x" + vHeight + ")";
            if (!vBitDepth.isEmpty()) v += " " + vBitDepth;
            if (!vHdr.isEmpty()) v += " " + vHdr;
            videoList << v;
        }
        vFormat.clear(); vWidth.clear(); vHeight.clear(); vBitDepth.clear(); vHdr.clear();
    };

    auto pushAudio = [&]() {
        if (!aFormat.isEmpty()) {
            QString a = aFormat;
            if (!aChannels.isEmpty()) a += " " + aChannels;
            if (!aBitrate.isEmpty()) a += " @ " + aBitrate;
            if (!aLang.isEmpty()) a += " [" + aLang + "]";
            audioList << a;
        }
        aFormat.clear(); aChannels.clear(); aLang.clear(); aBitrate.clear();
    };

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            if (currentSection == Video) pushVideo();
            else if (currentSection == Audio) pushAudio();
            continue;
        }

        if (!trimmed.contains(":")) {
            if (currentSection == Video) pushVideo();
            else if (currentSection == Audio) pushAudio();

            if (trimmed.startsWith("General")) currentSection = General;
            else if (trimmed.startsWith("Video")) currentSection = Video;
            else if (trimmed.startsWith("Audio")) currentSection = Audio;
            else if (trimmed.startsWith("Text")) currentSection = Text;
            else currentSection = None;
            continue;
        }

        int colonIdx = trimmed.indexOf(':');
        QString key = trimmed.left(colonIdx).trimmed();
        QString val = trimmed.mid(colonIdx + 1).trimmed();

        if (currentSection == General) {
            if (key == "Overall bit rate") overallBitrate = val;
        } else if (currentSection == Video) {
            if (key == "Format") vFormat = val;
            else if (key == "Width") vWidth = val.replace(" pixels", "").replace(" ", ""); 
            else if (key == "Height") vHeight = val.replace(" pixels", "").replace(" ", "");
            else if (key == "Bit depth") vBitDepth = val;
            else if (key == "HDR format") vHdr = val;
        } else if (currentSection == Audio) {
            if (key == "Format") aFormat = val;
            else if (key == "Channel(s)") aChannels = val.replace(" channels", "ch").replace(" channel", "ch");
            else if (key == "Bit rate") aBitrate = val;
            else if (key == "Language") aLang = val;
        } else if (currentSection == Text) {
            if (key == "Language" && !subList.contains(val)) subList << val;
        }
    }
    
    if (currentSection == Video) pushVideo();
    else if (currentSection == Audio) pushAudio();

    if (videoList.isEmpty()) return ""; 

    QString vOut = videoList.join(", ");
    QString aOut = audioList.isEmpty() ? "Unknown" : audioList.join("<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
    QString sOut = subList.isEmpty() ? "None Detected" : subList.join(", ");
    
    QString html = "<span style='color: #7aa2f7;'><b>📺 Video:</b></span> " + vOut + "<br>" +
                   "<span style='color: #7aa2f7;'><b>🔊 Audio:</b></span> " + aOut + "<br>" +
                   "<span style='color: #7aa2f7;'><b>💬 Subs:</b></span> " + sOut + "<br>" +
                   "<span style='color: #7aa2f7;'><b>📊 Bitrate:</b></span> " + overallBitrate + "<br><br>" +
                   "<span style='color: #565f89; font-size: 11px;'>📄 " + filename + "</span>";
                   
    return html;
}

QString FileView::formatExtendedMediaInfo(const QString &filename) {
    QString lower = filename.toLower();
    QStringList video, audio, subs;
    
    if (lower.contains("2160p") || lower.contains("4k")) video << "4K UHD";
    else if (lower.contains("1080p")) video << "1080p";
    else if (lower.contains("720p")) video << "720p";
    
    if (lower.contains("hevc") || lower.contains("x265") || lower.contains("h265")) video << "HEVC/x265";
    else if (lower.contains("x264") || lower.contains("h264")) video << "AVC/x264";
    
    if (lower.contains("dv") || lower.contains("dovi")) video << "Dolby Vision";
    if (lower.contains("hdr") || lower.contains("10bit")) video << "HDR10";
    
    if (lower.contains("atmos")) audio << "Dolby Atmos";
    if (lower.contains("truehd")) audio << "TrueHD";
    if (lower.contains("dts-hd") || lower.contains("dtshd") || lower.contains("dts-hdma")) audio << "DTS-HD MA";
    else if (lower.contains("dts")) audio << "DTS";
    if (lower.contains("eac3") || lower.contains("dd+")) audio << "EAC3 (DD+)";
    else if (lower.contains("ac3") || lower.contains("dd5.1") || lower.contains("5.1")) audio << "AC3 5.1";
    if (lower.contains("aac")) audio << "AAC";
    if (lower.contains("flac")) audio << "FLAC";
    
    if (lower.contains("dual audio") || lower.contains("dual-audio")) audio << "Dual Audio";
    else if (lower.contains("multi audio") || lower.contains("multi-audio")) audio << "Multi Audio";
    
    if (lower.contains("multi sub") || lower.contains("multisub") || lower.contains("multi-sub")) subs << "Multiple Languages";
    else if (lower.contains("esub") || lower.contains("e-sub")) subs << "English Subbed";
    
    QString vStr = video.isEmpty() ? "Unknown" : video.join(" • ");
    QString aStr = audio.isEmpty() ? "Unknown" : audio.join(" • ");
    QString sStr = subs.isEmpty() ? "None Detected" : subs.join(" • ");
    
    QString html = "<span style='color: #7aa2f7;'><b>📺 Video:</b></span> " + vStr + "<br>" +
                   "<span style='color: #7aa2f7;'><b>🔊 Audio:</b></span> " + aStr + "<br>" +
                   "<span style='color: #7aa2f7;'><b>💬 Subs:</b></span> " + sStr + "<br>" +
                   "<span style='color: #7aa2f7;'><b>📊 Bitrate:</b></span> Probe Unavailable<br><br>" +
                   "<span style='color: #565f89; font-size: 11px;'>📄 " + filename + "</span>";
                   
    return html;
}

QString FileView::formatMediaQuality(const QString &filename) {
    QStringList tags;
    QString lower = filename.toLower();
    if (lower.contains("2160p") || lower.contains("4k")) tags << "4K UHD";
    else if (lower.contains("1080p")) tags << "1080p";
    else if (lower.contains("720p")) tags << "720p";
    if (lower.contains("hevc") || lower.contains("x265") || lower.contains("h265")) tags << "HEVC";
    else if (lower.contains("x264") || lower.contains("h264")) tags << "x264";
    if (lower.contains("hdr") || lower.contains("10bit")) tags << "HDR";
    return tags.isEmpty() ? "Standard Definition" : tags.join(" • ");
}

void FileView::setMounts(const QStringList &mountPaths) {
    m_currentMounts = mountPaths;
    stopScan();
    
    m_view->hide();
    m_emptyTextLabel->setText("Loading library from cache...");
    
    QIcon icon(":/icons/hourglass.svg");
    QPixmap hourglass = icon.pixmap(80, 80); 

    if (!hourglass.isNull()) {
        QPainter painter(&hourglass);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(hourglass.rect(), QColor("#7aa2f7"));
        painter.end();
        
        m_emptyIconLabel->setPixmap(hourglass); 
    } else {
        m_emptyIconLabel->setText("⏳");
    }

    m_emptyWidget->show();

    QCoreApplication::processEvents();

    QTimer::singleShot(50, this, [this]() {
        loadFromCache();
    });
}

void FileView::loadFromCache() {
    m_isDetailView = false;
    if (m_backBtn) m_backBtn->hide();
    if (m_titleLabel) m_titleLabel->setText("Library");

    QFile file(m_cacheFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        m_model->clear();
        m_knownPaths.clear();
        m_mediaGroups.clear();
        m_groupItems.clear();

        for (const QString &mount : m_currentMounts) {
    if (root.contains(mount)) {
        QJsonArray arr = root[mount].toArray();
        QList<VideoFile> batch;

        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            batch.append({obj["name"].toString(), obj["path"].toString(), mount});

            if (i > 0 && i % 300 == 0)
                QCoreApplication::processEvents();
        }

        if (!batch.isEmpty()) {
            onBatchFound(batch);
            QCoreApplication::processEvents();
        }
    }
}
        file.close();
    }

    if (m_model->rowCount() == 0) {
        m_view->hide();
        clearSidebar();
        m_emptyIconLabel->clear(); 
        
        if (m_currentMounts.isEmpty()) {
            m_emptyTextLabel->setText("No drives mounted.\nGo to the Drives tab to mount one!");
            
            QPixmap pix = QIcon(":/icons/empty.svg").pixmap(80, 80);
            if (!pix.isNull()) {
                QImage img = pix.toImage();
                img.invertPixels(QImage::InvertRgb);
                m_emptyIconLabel->setPixmap(QPixmap::fromImage(img));
                QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
                opacityEffect->setOpacity(0.50);
                m_emptyIconLabel->setGraphicsEffect(opacityEffect);
            }
        } else {
            m_emptyTextLabel->setText("Cloud connecting...\nAuto-scanning in a few seconds...");
            
            QPixmap pix = QIcon(":/icons/drives.svg").pixmap(80, 80);
            if (!pix.isNull()) {
                QImage img = pix.toImage();
                img.invertPixels(QImage::InvertRgb);
                m_emptyIconLabel->setPixmap(QPixmap::fromImage(img));
            }
            
            QTimer::singleShot(3500, this, [this](){ startScan(); });
        }
        m_emptyWidget->show();
    } else {
        m_emptyWidget->hide();
        m_view->show();
        updateGridSize();
    }
}

void FileView::appendToCache(const QList<VideoFile> &files) {
    QFile file(m_cacheFilePath);
    QJsonObject root;
    if (file.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
    }
    for (const VideoFile &vf : files) {
        QJsonArray arr = root[vf.mountPath].toArray();
        bool exists = false;
        for (const QJsonValue &v : arr) if (v.toObject()["path"].toString() == vf.path) { exists = true; break; }
        if (!exists) {
            QJsonObject obj; obj["name"] = vf.name; obj["path"] = vf.path;
            arr.append(obj); root[vf.mountPath] = arr;
        }
    }
    if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(root).toJson());
}

void FileView::showRescanMenu() {
    if (m_currentMounts.isEmpty()) { m_toast->showMessage("No drives mounted to scan!"); return; }
    QMenu menu(this);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu.setStyleSheet(R"(
        QMenu { background-color: #1f2335; color: #c0caf5; border: 1px solid #414868; border-radius: 12px; padding: 6px; }
        QMenu::item { padding: 12px 35px 12px 20px; border-radius: 8px; font-weight: 600; font-size: 13px; margin: 2px 0; }
        QMenu::item:selected { background-color: #7aa2f7; color: #1a1b26; }
        QMenu::separator { height: 1px; background: #2f3549; margin: 6px 15px; }
    )");

    for (const QString &mount : m_currentMounts) {
        QString driveName = QDir(mount).dirName(); 
        QAction *act = menu.addAction(QString("Rescan %1").arg(driveName));
        connect(act, &QAction::triggered, this, [this, mount]() { startScan(QStringList() << mount); });
    }
    if (m_currentMounts.size() > 1) {
        menu.addSeparator();
        QAction *allAct = menu.addAction("Rescan All Mounted");
        connect(allAct, &QAction::triggered, this, [this]() { startScan(m_currentMounts); });
    }
    menu.exec(m_rescanBtn->mapToGlobal(QPoint(0, m_rescanBtn->height() + 8)));
}

void FileView::startScan(const QStringList &targets) {
    QStringList scanTargets = targets.isEmpty() ? m_currentMounts : targets;
    if (scanTargets.isEmpty()) return;
    stopScan();

    QSet<QString> targetSet(scanTargets.begin(), scanTargets.end());
    QSet<QString> mountSet(m_currentMounts.begin(), m_currentMounts.end());

    if (targetSet == mountSet || m_model->rowCount() == 0) {
        m_model->clear();
        m_knownPaths.clear();
        m_mediaGroups.clear();
        m_groupItems.clear();
        clearSidebar();
    }

    m_isDetailView = false;
    m_backBtn->hide();
    m_titleLabel->setText("Library");
    m_toast->showMessage((scanTargets.size() > 1) ? "Scanning all drives..." : "Scanning drive...");

    m_scanThread = new QThread;
    m_scanWorker = new ScanWorker(scanTargets);
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanThread, &QThread::started, m_scanWorker, &ScanWorker::process);
    connect(m_scanWorker, &ScanWorker::batchFound, this, &FileView::onBatchFound);
    connect(m_scanWorker, &ScanWorker::finished, this, &FileView::onScanFinished);
    connect(m_scanThread, &QThread::finished, m_scanThread, &QObject::deleteLater);
    connect(m_scanThread, &QThread::finished, m_scanWorker, &QObject::deleteLater);
    m_scanThread->start();
}

void FileView::stopScan() {
    if (m_scanWorker) m_scanWorker->requestStop();
    if (m_scanThread) { m_scanThread->quit(); m_scanThread->wait(); }
    m_scanWorker = nullptr; m_scanThread = nullptr;
}

void FileView::onBatchFound(const QList<VideoFile> &files) {
    for (const VideoFile &vf : files) {
        if (!m_knownPaths.contains(vf.path)) {
            MediaInfo info = MediaParser::parse(vf.name, vf.path);
            
            QString stackKey;
            if (info.isSeries) stackKey = "series_" + info.seriesName.toLower();
            else stackKey = "movie_" + info.title.toLower() + "_" + info.year; 
            
            m_mediaGroups[stackKey].append(vf);

            if (!m_groupItems.contains(stackKey)) {
                QStandardItem *item = new QStandardItem(info.isSeries ? info.seriesName : info.title);
                item->setData(stackKey, FilePathRole);
                
                item->setData(info.isSeries, IsStackRole); 
                item->setData(true, IsDefaultIconRole);
                
                if (info.isSeries) {
                    item->setData("1 Episode", SubtitleRole);
                } else if (!info.year.isEmpty()) {
                    item->setData(info.year, SubtitleRole);
                }
                
                item->setIcon(QIcon(":/icons/default.svg"));
                m_model->appendRow(item);
                m_groupItems[stackKey] = item;
                
                m_tmdb->fetchPoster(info, stackKey); 
            } else {
                if (info.isSeries) {
                    QSet<QString> uniqueEpisodes;
                    for (const auto &f : m_mediaGroups[stackKey]) {
                        MediaInfo epInfo = MediaParser::parse(f.name, f.path);
                        QString epKey = epInfo.seasonEpisode.isEmpty() ? f.name : epInfo.seasonEpisode;
                        uniqueEpisodes.insert(epKey);
                    }
                    m_groupItems[stackKey]->setData(QString("%1 Episodes").arg(uniqueEpisodes.size()), SubtitleRole);
                } else {
                    int count = m_mediaGroups[stackKey].size();
                    m_groupItems[stackKey]->setData(QString("%1 Versions").arg(count), SubtitleRole);
                }
            }
            m_knownPaths.insert(vf.path);
        }
    }
    
    appendToCache(files);
    
    if (m_model->rowCount() > 0) {
        m_emptyWidget->hide();
        m_view->show();
        updateGridSize();
    }
}

void FileView::updateSidebar(const QModelIndex &index) {
    if (!index.isValid()) {
        clearSidebar();
        return;
    }

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    m_sidebarPoster->setPixmap(icon.pixmap(410, 615));

    QString tmdbJsonStr = index.data(TmdbDataRole).toString();
    if (!tmdbJsonStr.isEmpty()) {
        QJsonObject tmdb = QJsonDocument::fromJson(tmdbJsonStr.toUtf8()).object();
        
        QString title = tmdb["title"].toString();
        if (title.isEmpty()) title = tmdb["name"].toString();
        if (title.isEmpty()) title = index.data(Qt::DisplayRole).toString();
        m_sidebarTitle->setText(title);
        
        QString rawDate = tmdb["release_date"].toString();
        if (rawDate.isEmpty()) rawDate = tmdb["first_air_date"].toString();
        QDate date = QDate::fromString(rawDate, "yyyy-MM-dd");
        QString displayDate = date.isValid() ? date.toString("dd-MM-yyyy") : "Unknown Date";
        
        double rating = tmdb["vote_average"].toDouble();
        
        QString metaText;
        if (!displayDate.isEmpty()) metaText += "📅 Released: " + displayDate + "  ";
        if (rating > 0) metaText += QString("⭐ %1/10").arg(rating, 0, 'f', 1);
        m_sidebarMeta->setText(metaText);

        QString overview = tmdb["overview"].toString();
        m_sidebarOverview->setText(overview.isEmpty() ? "No description available." : overview);
    } else {
        m_sidebarTitle->setText(index.data(Qt::DisplayRole).toString());
        m_sidebarMeta->setText(index.data(SubtitleRole).toString());
        m_sidebarOverview->setText("Loading details from TMDB...");
    }

    m_sidebarPlayBtn->setEnabled(true);
}

void FileView::onItemClicked(const QModelIndex &index) {
    QString id = index.data(FilePathRole).toString(); 
    bool isStack = index.data(IsStackRole).toBool();

    updateSidebar(index); 

    QString dialogStyle = R"(
        QDialog { background-color: #16161e; border: 1px solid #2f3549; border-radius: 8px; }
        QListWidget { background: #1f2335; color: #c0caf5; border: 1px solid #2f3549; border-radius: 6px; padding: 4px; outline: none; }
        QListWidget::item { padding: 12px; border-bottom: 1px solid #16161e; border-radius: 4px; }
        QListWidget::item:selected { background: #2f3549; color: #7aa2f7; font-weight: bold; }
        QListWidget::item:hover:!selected { background: #292e42; }
        
        QPushButton { background: #2f3549; color: #c0caf5; border-radius: 6px; padding: 8px 24px; font-weight: bold; font-size: 13px; border: none; }
        QPushButton:hover { background: #414868; }
        QPushButton#playBtn { background: #7aa2f7; color: #1a1b26; }
        QPushButton#playBtn:hover { background: #89b4fa; }


        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 2px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 8px; margin: 2px; }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: #414868; border-radius: 4px; min-height: 20px; min-width: 20px; }
        QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { background: #7aa2f7; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; }
        QScrollBar::add-page, QScrollBar::sub-page { background: none; }
    )";

    if (m_isDetailView) {
        QStringList multiPaths = index.data(Qt::UserRole + 50).toStringList();
        QStringList multiNames = index.data(Qt::UserRole + 51).toStringList();

        m_currentPlayTitle = index.data(Qt::DisplayRole).toString();
        QVariant v = index.data(BackdropRole);
        if (v.canConvert<QPixmap>()) m_currentBackdrop = v.value<QPixmap>();

        if (multiPaths.size() > 1) {
            QDialog dialog(this);
            dialog.setWindowTitle("Select Version");
            dialog.setMinimumWidth(600);
            dialog.setMinimumHeight(350);
            dialog.setStyleSheet(dialogStyle);

            QVBoxLayout *layout = new QVBoxLayout(&dialog);
            layout->setContentsMargins(15, 15, 15, 15);
            layout->setSpacing(15);

            QListWidget *list = new QListWidget();
            for (const QString &n : multiNames) list->addItem(n);
            list->setCurrentRow(0); 
            connect(list, &QListWidget::itemDoubleClicked, [&dialog]() { dialog.accept(); });
            layout->addWidget(list);
            
            QHBoxLayout *btnLayout = new QHBoxLayout();
            btnLayout->addStretch();
            
            QPushButton *cancelBtn = new QPushButton("Cancel");
            QPushButton *playBtn = new QPushButton("Play");
            playBtn->setObjectName("playBtn");
            
            connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
            connect(playBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
            
            btnLayout->addWidget(cancelBtn);
            btnLayout->addWidget(playBtn);
            layout->addLayout(btnLayout);

            if (dialog.exec() == QDialog::Accepted && list->currentRow() >= 0) {
                m_currentPlayPath = multiPaths[list->currentRow()];
                requestMediaInfo(m_currentPlayPath, m_currentPlayTitle);
            }
        } else {
            m_currentPlayPath = multiPaths.isEmpty() ? id : multiPaths.first();
            requestMediaInfo(m_currentPlayPath, m_currentPlayTitle);
        }
        
    } else if (isStack) {
        QVariant parentTmdb = index.data(TmdbDataRole);
        QVariant parentBackdrop = index.data(BackdropRole);
        QIcon parentIcon = index.data(Qt::DecorationRole).value<QIcon>(); 
        m_searchBar->clear();

        m_mainLibraryItems.clear();
        while (m_model->rowCount() > 0) m_mainLibraryItems.append(m_model->takeRow(0).at(0));
        
        m_isDetailView = true;
        m_backBtn->show();

        QList<VideoFile> files = m_mediaGroups[id];
        
        QMap<QString, QList<VideoFile>> episodeGroups;
        for (const auto &f : files) {
            MediaInfo info = MediaParser::parse(f.name, f.path);
            QString epKey = info.isSeries ? info.seasonEpisode : f.name;
            if (epKey.isEmpty()) epKey = f.name; 
            episodeGroups[epKey].append(f);
        }

        QStringList epKeys = episodeGroups.keys();
        epKeys.sort();

        for (const QString &epKey : epKeys) {
            QList<VideoFile> epFiles = episodeGroups[epKey];
            
            QStringList multiPaths, multiNames;
            for (const auto &ef : epFiles) {
                multiPaths.append(ef.path);
                multiNames.append(ef.name);
            }
            
            QStandardItem *subItem = new QStandardItem(epKey);
            subItem->setData(multiPaths.first(), FilePathRole); 
            subItem->setData(multiPaths, Qt::UserRole + 50);    
            subItem->setData(multiNames, Qt::UserRole + 51);    
            subItem->setData(false, IsStackRole);
            subItem->setData(false, IsDefaultIconRole);

            if (!epFiles.isEmpty()) {
                MediaInfo info = MediaParser::parse(epFiles.first().name, epFiles.first().path);
                if (info.isSeries) {
                    subItem->setData(info.seasonNumber, SeasonRole);
                    subItem->setData(info.episodeNumber, EpisodeRole);
                }
            }
            
            if (epFiles.size() > 1) {
                subItem->setData(QString("%1 Versions").arg(epFiles.size()), SubtitleRole);
            } else {
                subItem->setData(formatMediaQuality(epFiles.first().name), SubtitleRole);
            }
            
            subItem->setIcon(parentIcon);
            subItem->setData(parentBackdrop, BackdropRole);
            subItem->setData(parentTmdb, TmdbDataRole); 
            m_model->appendRow(subItem);
        }
        updateGridSize();
        m_view->verticalScrollBar()->setValue(0);
        
        if (!epKeys.isEmpty()) {
            QString firstEp = epKeys.first();
            m_currentPlayPath = episodeGroups[firstEp].first().path;
            m_currentPlayTitle = m_model->item(0)->text();
            m_currentBackdrop = parentBackdrop.value<QPixmap>();
            updateSidebar(m_proxy->mapFromSource(m_model->index(0, 0))); 
            requestMediaInfo(m_currentPlayPath, episodeGroups[firstEp].first().name);
        }
            
    } else {
        if (m_mediaGroups.contains(id) && !m_mediaGroups[id].isEmpty()) {
            QList<VideoFile> movieFiles = m_mediaGroups[id];
            m_currentPlayTitle = index.data(Qt::DisplayRole).toString();
            QVariant v = index.data(BackdropRole);
            if (v.canConvert<QPixmap>()) m_currentBackdrop = v.value<QPixmap>();

            if (movieFiles.size() > 1) {
                QDialog dialog(this);
                dialog.setWindowTitle("Select Version");
                dialog.setMinimumWidth(600);
                dialog.setMinimumHeight(350);
                dialog.setStyleSheet(dialogStyle);

                QVBoxLayout *layout = new QVBoxLayout(&dialog);
                layout->setContentsMargins(15, 15, 15, 15);
                layout->setSpacing(15);

                QListWidget *list = new QListWidget();
                for (const auto &f : movieFiles) list->addItem(f.name);
                list->setCurrentRow(0);
                connect(list, &QListWidget::itemDoubleClicked, [&dialog]() { dialog.accept(); });
                layout->addWidget(list);
                
                QHBoxLayout *btnLayout = new QHBoxLayout();
                btnLayout->addStretch();
                
                QPushButton *cancelBtn = new QPushButton("Cancel");
                QPushButton *playBtn = new QPushButton("Play");
                playBtn->setObjectName("playBtn");
                
                connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
                connect(playBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
                
                btnLayout->addWidget(cancelBtn);
                btnLayout->addWidget(playBtn);
                layout->addLayout(btnLayout);

                if (dialog.exec() == QDialog::Accepted && list->currentRow() >= 0) {
                    m_currentPlayPath = movieFiles[list->currentRow()].path;
                    requestMediaInfo(m_currentPlayPath, m_currentPlayTitle);
                }
            } else {
                m_currentPlayPath = movieFiles[0].path; 
                requestMediaInfo(m_currentPlayPath, m_currentPlayTitle);
            }
        }
    }
}

void FileView::onPlayButtonClicked() {
    if (!m_currentPlayPath.isEmpty()) {
        emit playVideoRequested(m_currentPlayPath, m_currentPlayTitle, m_currentBackdrop);
    }
}

void FileView::onBackClicked() {
    if (!m_isDetailView) return;
    
    m_isDetailView = false;
    m_backBtn->hide();
    m_titleLabel->setText("Library");
    
    m_model->clear();
    for (auto *item : m_mainLibraryItems) {
        m_model->appendRow(item);
    }
    m_mainLibraryItems.clear();
    
    updateGridSize();
    
    QModelIndex firstIndex = m_proxy->index(0, 0);
    if (firstIndex.isValid()) {
        m_view->setCurrentIndex(firstIndex);
        updateSidebar(firstIndex); 
    }
}

void FileView::onMetadataLoaded(const QString &path, const QJsonObject &metadata) {
    QString jsonStr = QJsonDocument(metadata).toJson(QJsonDocument::Compact);
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem *item = m_model->item(i);
        if (item->data(FilePathRole).toString() == path) {
            item->setData(jsonStr, TmdbDataRole);
            
            QModelIndex proxyIndex = m_proxy->mapFromSource(item->index());
            if (m_view->currentIndex() == proxyIndex) updateSidebar(proxyIndex);
            return;
        }
    }
}

void FileView::onPosterLoaded(const QString &path, const QPixmap &poster) {
    if (poster.isNull()) return;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem *item = m_model->item(i);
        if (item->data(FilePathRole).toString() == path) {
            item->setIcon(QIcon(poster));
            item->setData(false, IsDefaultIconRole);
            
            QModelIndex proxyIndex = m_proxy->mapFromSource(item->index());
            if (m_view->currentIndex() == proxyIndex) updateSidebar(proxyIndex);
            return;
        }
    }
}

void FileView::onBackdropLoaded(const QString &path, const QPixmap &backdrop) {
    if (backdrop.isNull()) return;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem *item = m_model->item(i);
        if (item->data(FilePathRole).toString() == path) {
            item->setData(QVariant::fromValue(backdrop), BackdropRole);
            return;
        }
    }
}

void FileView::updateGridSize() {
    QTimer::singleShot(20, this, [this](){
        if (!m_view->isVisible()) return;
        MediaDelegate *delegate = qobject_cast<MediaDelegate*>(m_view->itemDelegate());
        if (delegate) {
            int availableWidth = m_view->viewport()->width() - 60; 
            int columns = qMax(1, availableWidth / 200); 
            int exactCardWidth = (availableWidth / columns) - 15; 
            delegate->setCardWidth(exactCardWidth);
            m_view->doItemsLayout();
        }
    });
}

void FileView::refreshMetadata() {
    for (auto it = m_mediaGroups.constBegin(); it != m_mediaGroups.constEnd(); ++it) {
        QString stackKey = it.key();
        QStandardItem *item = m_groupItems.value(stackKey);
        
        if (item && item->data(IsDefaultIconRole).toBool()) {
            const QList<VideoFile> &files = it.value();
            if (!files.isEmpty()) {
                MediaInfo info = MediaParser::parse(files.first().name, files.first().path);
                m_tmdb->fetchPoster(info, stackKey);
            }
        }
    }
}

void FileView::onScanFinished() { m_toast->showMessage("Scan Complete!"); }
void FileView::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); updateGridSize(); }
bool FileView::eventFilter(QObject *o, QEvent *e) { if (o == m_view->viewport() && e->type() == QEvent::Resize) updateGridSize(); return QWidget::eventFilter(o, e); }