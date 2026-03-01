#include "drivespage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QCheckBox> 
#include <QSettings> 
#include <QTimer> 
#include <QProcess> 
#include <QScrollBar>
#include <QPixmap>
#include <QIcon>
#include <QImage>
#include <QSize>
void cleanUpStuckMount(const QString &path) {
    if (path.isEmpty()) return;
    QDir dir(path);
    if (!dir.exists()) return;

#ifdef Q_OS_WIN
    dir.rmdir(path);
#else
    QProcess unmounter;
    unmounter.start("fusermount", QStringList() << "-u" << "-z" << path);
    unmounter.waitForFinished(1000);

    QProcess fallback;
    fallback.start("umount", QStringList() << "-l" << path);
    fallback.waitForFinished(1000);

    dir.rmdir(path);
#endif
}

DrivesPage::DrivesPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget *headerContainer = new QWidget(this);
    headerContainer->setFixedHeight(100);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(40, 40, 40, 20);
    headerLayout->setSpacing(20);
    headerLayout->setAlignment(Qt::AlignVCenter);

    QLabel *header = new QLabel("Cloud Drives", headerContainer);
    header->setStyleSheet("font-size: 32px; font-weight: 800; color: #7aa2f7; background: transparent; border: none;");
    headerLayout->addWidget(header);
    headerLayout->addStretch();
    layout->addWidget(headerContainer);

    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(40, 0, 40, 40);
    contentLayout->setSpacing(20);

    m_statusLabel = new QLabel("Scanning...", contentWidget);
    m_statusLabel->setStyleSheet("color: #565f89; font-style: italic;");
    contentLayout->addWidget(m_statusLabel);

    m_list = new QListWidget(contentWidget);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->verticalScrollBar()->setSingleStep(20);
    m_list->setFlow(QListView::LeftToRight); 
    m_list->setWrapping(true);
    m_list->setSpacing(15);
    m_list->setStyleSheet(R"(
        QListWidget { background: transparent; outline: none; border: none; }
        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
        QScrollBar:handle:vertical { background: #414868; min-height: 40px; border-radius: 4px; }
        QScrollBar:handle:vertical:hover { background: #7aa2f7; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");
    contentLayout->addWidget(m_list);
    
    layout->addWidget(contentWidget, 1);

    loadRemotes();

    QTimer::singleShot(200, this, [this]() {
        QSettings settings("Kino", "AutoMount");
        for (auto it = m_drives.begin(); it != m_drives.end(); ++it) {
            if (settings.value(it.key(), false).toBool() && !it.value().isMounted) {
                startMount(it.key());
            }
        }
    });
}

DrivesPage::~DrivesPage() {
    for (auto it = m_drives.begin(); it != m_drives.end(); ++it) {
        if (it.value().isMounted) stopMount(it.key());
    }
}

void DrivesPage::refresh() { loadRemotes(); }

QStringList DrivesPage::getActiveMountPaths() const {
    QStringList paths;
    for (auto it = m_drives.begin(); it != m_drives.end(); ++it) {
        if (it.value().isMounted) paths.append(it.value().mountPath);
    }
    return paths;
}

QString DrivesPage::findRcloneConfig() {
    QStringList paths;
    QSettings settings("Kino", "AppConfig");
    QString savedPath = settings.value("rclone_conf_path", "").toString();
    if (!savedPath.isEmpty()) paths.append(savedPath);

#ifdef Q_OS_WIN
    paths.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/rclone/rclone.conf");
#else
    paths.append(QDir::homePath() + "/.config/rclone/rclone.conf");
#endif

    paths.append(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/rclone/rclone.conf");

    for (const QString &path : paths) {
        if (QFile::exists(path)) return path;
    }
    return QString();
}

void DrivesPage::loadRemotes() {
    m_list->clear();
    QString configPath = findRcloneConfig();
    
    if (configPath.isEmpty()) {
        m_statusLabel->setText("Error: rclone.conf not found!"); 
        return;
    }
    
    m_statusLabel->setText("Loaded from: " + QDir::toNativeSeparators(configPath)); 
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith('[') && line.endsWith(']')) {
            QString name = line.mid(1, line.length() - 2);
            if (!name.isEmpty() && !name.contains("cache")) addDriveCard(name);
        }
    }
}

void DrivesPage::addDriveCard(const QString &name) {
    QWidget *card = new QWidget();
    card->setFixedSize(240, 150);
    card->setObjectName("driveCard");
    
    card->setStyleSheet(R"(
        QWidget#driveCard { 
            background-color: #1f2335; 
            border-radius: 12px; 
            border: 1px solid #292e42; 
        }
        QWidget#driveCard:hover {
            border: 1px solid #7aa2f7;
            background-color: #24283b;
        }
    )");

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 20, 20, 20);

    QLabel *nameLbl = new QLabel(name);
    nameLbl->setStyleSheet("color: #7aa2f7; font-size: 18px; font-weight: bold; background: transparent;");
    lay->addWidget(nameLbl);

    QCheckBox *autoCb = new QCheckBox("Auto-Mount on Startup");
    autoCb->setCursor(Qt::PointingHandCursor);
    autoCb->setStyleSheet(R"(
        QCheckBox { color: #a9b1d6; background: transparent; font-size: 12px; font-weight: bold; }
        QCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px; border: 1px solid #414868; background: #1a1b26; }
        QCheckBox::indicator:checked { background: #7aa2f7; border: 1px solid #7aa2f7; image: url(:/icons/check.svg); }
    )");
    
    QSettings settings("Kino", "AutoMount");
    autoCb->setChecked(settings.value(name, false).toBool());
    connect(autoCb, &QCheckBox::toggled, [name](bool c){ QSettings s("Kino", "AutoMount"); s.setValue(name, c); });
    lay->addWidget(autoCb);

    lay->addStretch();

    QHBoxLayout *btnLay = new QHBoxLayout();
    QPushButton *mntBtn = new QPushButton("Mount Drive");
    mntBtn->setCursor(Qt::PointingHandCursor);
    mntBtn->setFixedHeight(36);
    
    QPushButton *scanBtn = new QPushButton();
    QPixmap scanPix = QIcon(":/icons/search.svg").pixmap(24, 24);
    QImage img = scanPix.toImage();
        img.invertPixels(QImage::InvertRgb);
        scanBtn->setIcon(QIcon(QPixmap::fromImage(img)));
        scanBtn->setIconSize(QSize(20, 20));
    scanBtn->setCursor(Qt::PointingHandCursor);
    scanBtn->setStyleSheet("QPushButton { background: #1a1b26; border-radius: 8px; border: 1px solid #414868; color: white; font-size: 16px; } QPushButton:hover { background: #414868; border: 1px solid #7aa2f7; }");
    
    m_drives[name].btn = mntBtn; 
    scanBtn->setVisible(m_drives[name].isMounted);

    connect(mntBtn, &QPushButton::clicked, [this, name](){ toggleMount(name); });
    connect(scanBtn, &QPushButton::clicked, [this, name](){ 
        if (m_drives[name].isMounted) emit scanRequested(m_drives[name].mountPath); 
    });

    if (m_drives[name].isMounted) {
        mntBtn->setText("Unmount");
        mntBtn->setStyleSheet("background: #f7768e; color: #1a1b26; border-radius: 8px; font-weight: bold; font-size: 14px; border: none;");
    } else {
        mntBtn->setText("Mount");
        mntBtn->setStyleSheet("background: #7aa2f7; color: #1a1b26; border-radius: 8px; font-weight: bold; font-size: 14px; border: none;");
    }

    btnLay->addWidget(mntBtn, 1);
    btnLay->addWidget(scanBtn);
    lay->addLayout(btnLay);

    QListWidgetItem *item = new QListWidgetItem(m_list);
    item->setSizeHint(card->size());
    m_list->setItemWidget(item, card);
}

void DrivesPage::toggleMount(const QString &remote) {
    if (m_drives[remote].isMounted) stopMount(remote);
    else startMount(remote);
    loadRemotes(); 
}

void DrivesPage::startMount(const QString &remote) {
    QString mountDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Kino_Mounts/" + remote;
    QDir().mkpath(mountDir);
    
    QProcess *proc = new QProcess(this);

#ifdef Q_OS_WIN
    QString execPath = "rclone.exe";
    QString nativeMountDir = QDir::toNativeSeparators(mountDir);
#else
    QString execPath = "rclone";
    QString nativeMountDir = mountDir;
#endif

    proc->start(execPath, QStringList() << "mount" << (remote + ":") << nativeMountDir << "--vfs-cache-mode" << "full");
    
    m_drives[remote].process = proc;
    m_drives[remote].mountPath = nativeMountDir;
    m_drives[remote].isMounted = true;
    
    if (m_drives[remote].btn) { 
        m_drives[remote].btn->setText("Unmount");
        m_drives[remote].btn->setStyleSheet("background-color: #f7768e; color: #1a1b26; border-radius: 6px; font-weight: bold;");
    }
    emit mountsChanged(); 
}

void DrivesPage::stopMount(const QString &remote) {
    DriveState &state = m_drives[remote];
    if (state.process) {
        state.process->terminate();
        state.process->waitForFinished(1000);
    }
    cleanUpStuckMount(state.mountPath);
    state.isMounted = false;
    
    if (state.btn) {
        state.btn->setText("Mount");
        state.btn->setStyleSheet("background-color: #7aa2f7; color: #1a1b26; border-radius: 6px; font-weight: bold;");
    }
    emit mountsChanged(); 
}