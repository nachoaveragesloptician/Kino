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
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPlainTextEdit>
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
    header->setStyleSheet("font-size: 28px; font-weight: 800; color: #c0caf5; background: transparent; border: none; letter-spacing: 0.5px;");
    headerLayout->addWidget(header);
    headerLayout->addStretch();
    layout->addWidget(headerContainer);

    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(40, 0, 40, 40);
    contentLayout->setSpacing(20);

    m_statusLabel = new QLabel("Scanning...", contentWidget);
    m_statusLabel->setStyleSheet("color: #565f89; font-size: 13px; font-style: italic;");
    contentLayout->addWidget(m_statusLabel);

    m_list = new QListWidget(contentWidget);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->verticalScrollBar()->setSingleStep(20);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setStyleSheet(R"(
        QListWidget { 
            background: rgba(30, 33, 46, 0.4); 
            outline: none; 
            border: 1px solid rgba(255, 255, 255, 0.05); 
            border-radius: 8px;
        }
        QListWidget::item { border-bottom: 1px solid rgba(255, 255, 255, 0.02); }
        QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
        QScrollBar:handle:vertical { background: #292e42; min-height: 40px; border-radius: 3px; }
        QScrollBar:handle:vertical:hover { background: #414868; }
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
    
    m_statusLabel->setText("Loaded config: " + QDir::toNativeSeparators(configPath)); 
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

void showFilterDialog(const QString &driveName, QWidget *parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Scanning Filters: " + driveName);
    dialog.setMinimumSize(550, 600);
    dialog.setStyleSheet(R"(
        QDialog { background-color: #1a1b26; color: #c0caf5; }
        QLabel { color: #c0caf5; font-weight: bold; font-size: 14px; }
        QPlainTextEdit { 
            background-color: #1f2335; color: white; border: 1px solid #414868; 
            border-radius: 6px; padding: 10px; font-family: monospace; font-size: 13px;
        }
        QPlainTextEdit:focus { border: 1px solid #7aa2f7; }
        QPushButton { 
            background: #3d59a1; color: white; border-radius: 4px; 
            padding: 8px 16px; font-weight: bold; border: none; font-size: 13px;
        }
        QPushButton:hover { background: #7aa2f7; color: #1a1b26; }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(25, 25, 25, 25);
    layout->setSpacing(15);

    QLabel *desc = new QLabel("Enter exact relative paths, one per line.\nLeave whitelist blank to scan all folders.");
    desc->setStyleSheet("color: #737aa2; font-style: italic; font-weight: normal; font-size: 13px;");
    layout->addWidget(desc);

    QLabel *whiteLabel = new QLabel("Whitelist (Scans ONLY these paths):");
    layout->addWidget(whiteLabel);

    QPlainTextEdit *whiteEdit = new QPlainTextEdit();
    whiteEdit->setPlaceholderText("Movies\nTV Shows\nAnime/Dubbed");
    layout->addWidget(whiteEdit, 1);

    QLabel *blackLabel = new QLabel("Blacklist (IGNORES these paths entirely):");
    layout->addWidget(blackLabel);

    QPlainTextEdit *blackEdit = new QPlainTextEdit();
    blackEdit->setPlaceholderText("Courses/Python\nPrivate/Photos\nExtras");
    layout->addWidget(blackEdit, 1);

    QSettings settings("Kino", "DriveFilters");
    whiteEdit->setPlainText(settings.value(driveName + "_whitelist", "").toString());
    blackEdit->setPlainText(settings.value(driveName + "_blacklist", "").toString());

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QObject::connect(box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(box);

    if (dialog.exec() == QDialog::Accepted) {
        settings.setValue(driveName + "_whitelist", whiteEdit->toPlainText().trimmed());
        settings.setValue(driveName + "_blacklist", blackEdit->toPlainText().trimmed());
    }
}

void DrivesPage::addDriveCard(const QString &name) {
    QWidget *rowWidget = new QWidget();
    rowWidget->setFixedHeight(64); 
    rowWidget->setObjectName("driveRow");
    
    rowWidget->setStyleSheet(R"(
        QWidget#driveRow { background-color: transparent; }
        QWidget#driveRow:hover { background-color: rgba(255, 255, 255, 0.02); }
    )");

    QHBoxLayout *lay = new QHBoxLayout(rowWidget);
    lay->setContentsMargins(20, 0, 20, 0);
    lay->setSpacing(20);

    QLabel *nameLbl = new QLabel(name);
    nameLbl->setStyleSheet("color: #c0caf5; font-size: 15px; font-weight: 600; background: transparent;");
    lay->addWidget(nameLbl);

    lay->addStretch(); 

    QCheckBox *autoCb = new QCheckBox("Auto-Mount");
    autoCb->setCursor(Qt::PointingHandCursor);
    autoCb->setStyleSheet(R"(
        QCheckBox { color: #737aa2; background: transparent; font-size: 13px; font-weight: 500; margin-right: 20px; }
        QCheckBox::indicator { width: 14px; height: 14px; border-radius: 3px; border: 1px solid #414868; background: #1a1b26; }
        QCheckBox::indicator:checked { background: #7aa2f7; border: 1px solid #7aa2f7; image: url(:/icons/check.svg); }
    )");
    
    QSettings settings("Kino", "AutoMount");
    autoCb->setChecked(settings.value(name, false).toBool());
    connect(autoCb, &QCheckBox::toggled, [name](bool c){ QSettings s("Kino", "AutoMount"); s.setValue(name, c); });
    lay->addWidget(autoCb);

    QWidget *btnContainer = new QWidget();
    btnContainer->setFixedWidth(120); 
    QHBoxLayout *btnLay = new QHBoxLayout(btnContainer);
    btnLay->setContentsMargins(0, 0, 0, 0);
    btnLay->setSpacing(10);
    
    QPushButton *filterBtn = new QPushButton();
    QPixmap filterPix = QIcon(":/icons/settings.svg").pixmap(24, 24);
    QImage fImg = filterPix.toImage();
    fImg.invertPixels(QImage::InvertRgb);
    filterBtn->setIcon(QIcon(QPixmap::fromImage(fImg)));
    filterBtn->setIconSize(QSize(16, 16));
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setFixedSize(32, 32);
    filterBtn->setStyleSheet(R"(
        QPushButton { background: transparent; border-radius: 4px; border: 1px solid #414868; color: white; } 
        QPushButton:hover { background: #292e42; border: 1px solid #e0af68; } 
    )");

    connect(filterBtn, &QPushButton::clicked, [this, name](){ 
        showFilterDialog(name, this); 
    });

    QPushButton *mntBtn = new QPushButton("Mount");
    mntBtn->setCursor(Qt::PointingHandCursor);
    mntBtn->setFixedHeight(32);
    
    m_drives[name].btn = mntBtn; 

    connect(mntBtn, &QPushButton::clicked, [this, name](){ toggleMount(name); });

    if (m_drives[name].isMounted) {
        mntBtn->setText("Unmount");
        mntBtn->setStyleSheet(R"(
            QPushButton { background: transparent; color: #f7768e; border: 1px solid rgba(247, 118, 142, 0.5); border-radius: 4px; font-weight: bold; font-size: 13px; }
            QPushButton:hover { background: rgba(247, 118, 142, 0.1); border: 1px solid #f7768e; }
        )");
    } else {
        mntBtn->setText("Mount");
        mntBtn->setStyleSheet(R"(
            QPushButton { background: transparent; color: #7aa2f7; border: 1px solid rgba(122, 162, 247, 0.5); border-radius: 4px; font-weight: bold; font-size: 13px; }
            QPushButton:hover { background: rgba(122, 162, 247, 0.1); border: 1px solid #7aa2f7; }
        )");
    }

    btnLay->addWidget(filterBtn);
    btnLay->addWidget(mntBtn, 1);
    lay->addWidget(btnContainer);

    QListWidgetItem *item = new QListWidgetItem(m_list);
    item->setSizeHint(rowWidget->size());
    m_list->setItemWidget(item, rowWidget);
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

    QStringList args;
    args << "mount" << (remote + ":") << nativeMountDir;

    QString confPath = findRcloneConfig();
    if (!confPath.isEmpty()) {
        args << "--config" << confPath;
    }

    QSettings settings("Kino", "AppConfig");
    QString customFlags = settings.value("rclone_flags", "").toString();
    
    if (!customFlags.isEmpty()) {
        args.append(customFlags.split(" ", Qt::SkipEmptyParts));
    } else {
        args << "--vfs-cache-mode" << "full";
    }

    proc->start(execPath, args);
    
    m_drives[remote].process = proc;
    m_drives[remote].mountPath = nativeMountDir;
    m_drives[remote].isMounted = true;
    
    if (m_drives[remote].btn) { 
        m_drives[remote].btn->setText("Unmount");
        m_drives[remote].btn->setStyleSheet(R"(
            QPushButton { background: transparent; color: #f7768e; border: 1px solid rgba(247, 118, 142, 0.5); border-radius: 4px; font-weight: bold; font-size: 13px; }
            QPushButton:hover { background: rgba(247, 118, 142, 0.1); border: 1px solid #f7768e; }
        )");
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
        state.btn->setStyleSheet(R"(
            QPushButton { background: transparent; color: #7aa2f7; border: 1px solid rgba(122, 162, 247, 0.5); border-radius: 4px; font-weight: bold; font-size: 13px; }
            QPushButton:hover { background: rgba(122, 162, 247, 0.1); border: 1px solid #7aa2f7; }
        )");
    }
    emit mountsChanged(); 
}