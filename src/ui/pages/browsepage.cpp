#include "browsepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QPainter>
#include <QMouseEvent>
#include <QLocale>
#include <QPushButton>

BrowseDelegate::BrowseDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void BrowseDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isHovered = option.state & QStyle::State_MouseOver;
    bool isSelected = option.state & QStyle::State_Selected;

    QRect rect = option.rect.adjusted(0, 2, 0, -2); 

    if (isSelected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(122, 162, 247, 40)); 
        painter->drawRoundedRect(rect, 8, 8);
    } else if (isHovered) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 255, 255, 10)); 
        painter->drawRoundedRect(rect, 8, 8);
    }

    const QFileSystemModel *model = qobject_cast<const QFileSystemModel*>(index.model());
    if (!model) { painter->restore(); return; }

    QFileInfo info = model->fileInfo(index);
    bool isDir = info.isDir();
    QString name = info.fileName();
    
    QIcon icon = isDir ? QIcon(":/icons/dir.svg") : QIcon(":/icons/default.svg"); 
    QRect iconRect(rect.left() + 15, rect.top() + (rect.height() - 24) / 2, 24, 24);
    
    QPixmap pix = icon.pixmap(24, 24);
    if(!pix.isNull()) {
        QPainter ip(&pix);
        ip.setCompositionMode(QPainter::CompositionMode_SourceIn);
        ip.fillRect(pix.rect(), QColor(isDir ? "#7aa2f7" : "#565f89")); 
        ip.end();
        painter->drawPixmap(iconRect, pix);
    }

    painter->setPen(QColor(isDir ? "#c0caf5" : "#a9b1d6"));
    QFont font = option.font;
    font.setPixelSize(14);
    font.setBold(isDir);
    painter->setFont(font);

    QRect textRect(iconRect.right() + 15, rect.top(), rect.width() - 150, rect.height());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    QString lowerName = name.toLower();
    bool isMedia = false;
    QStringList exts = {".mp4", ".mkv", ".avi", ".mov", ".webm", ".flv", ".wmv", ".m4v"};
    for (const QString &ext : exts) { if (lowerName.endsWith(ext)) { isMedia = true; break; } }

    if (!isDir) {
        if (isMedia && (isHovered || isSelected)) {
            QRect playRect(rect.right() - 45, rect.top() + (rect.height() - 28) / 2, 28, 28);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#7aa2f7"));
            painter->drawRoundedRect(playRect, 6, 6);
            
            QPixmap playPix(":/icons/play.svg");
            if(!playPix.isNull()) {
                QPainter pp(&playPix);
                pp.setCompositionMode(QPainter::CompositionMode_SourceIn);
                pp.fillRect(playPix.rect(), QColor("#1a1b26")); 
                pp.end();
                painter->drawPixmap(playRect.adjusted(6,6,-6,-6), playPix);
            } else {
                painter->setPen(QColor("#1a1b26"));
                painter->setFont(QFont("Segoe UI", 10, QFont::Bold));
                painter->drawText(playRect, Qt::AlignCenter, "▶");
            }
        } else {
            QString sizeStr = QLocale().formattedDataSize(info.size());
            painter->setPen(QColor("#565f89"));
            font.setPixelSize(12);
            font.setBold(false);
            painter->setFont(font);
            QRect sizeRect(rect.right() - 100, rect.top(), 80, rect.height());
            painter->drawText(sizeRect, Qt::AlignVCenter | Qt::AlignRight, sizeStr);
        }
    }

    painter->restore();
}

QSize BrowseDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    return QSize(option.rect.width(), 50); 
}



BrowsePage::BrowsePage(QWidget *parent) : QWidget(parent)
{
    m_rootPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Kino_Mounts";
    QDir().mkpath(m_rootPath);
    m_currentPath = m_rootPath;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget *headerContainer = new QWidget(this);
    headerContainer->setFixedHeight(100);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(40, 40, 40, 20);
    headerLayout->setSpacing(20);
    headerLayout->setAlignment(Qt::AlignVCenter);

    m_backBtn = new QPushButton("❮", headerContainer);
    m_backBtn->setFixedSize(40, 40);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(R"(
        QPushButton { background: rgba(255,255,255,0.08); color: white; border-radius: 20px; font-weight: bold; font-size: 18px; padding-right: 2px; border: 1px solid rgba(255,255,255,0.1); }
        QPushButton:hover { background: #7aa2f7; color: #1a1b26; border: none; }
    )");
    m_backBtn->hide();
    connect(m_backBtn, &QPushButton::clicked, this, [this](){
        QDir dir(m_currentPath);
        dir.cdUp();
        onDirectoryLoaded(dir.absolutePath());
    });
    headerLayout->addWidget(m_backBtn);

    m_pathLabel = new QLabel("Files", headerContainer);
    m_pathLabel->setStyleSheet("color: #7aa2f7; font-size: 32px; font-weight: 800; background: transparent; border: none;");
    headerLayout->addWidget(m_pathLabel);
    headerLayout->addStretch();

    QPushButton *refreshBtn = new QPushButton("Refresh", headerContainer);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setFixedHeight(40);
    refreshBtn->setStyleSheet(R"(
        QPushButton { background: #2f3549; color: #c0caf5; border-radius: 20px; padding: 0 20px; font-weight: bold; font-size: 14px; border: none; }
        QPushButton:hover { background: #414868; }
        QPushButton:pressed { background: #7aa2f7; color: #1a1b26; }
    )");
    connect(refreshBtn, &QPushButton::clicked, this, &BrowsePage::refresh);
    headerLayout->addWidget(refreshBtn);

    layout->addWidget(headerContainer);

    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(40, 0, 40, 40);
    contentLayout->setSpacing(15);

    m_emptyWidget = new QWidget(contentWidget);
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(15);
    
    QLabel *emptyIconLabel = new QLabel(m_emptyWidget);
    emptyIconLabel->setAlignment(Qt::AlignCenter);
    QIcon emptyIcon(":/icons/empty.svg");
    QPixmap emptyPix = emptyIcon.pixmap(80, 80);
    if (!emptyPix.isNull()) {
        QPainter p(&emptyPix);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(emptyPix.rect(), QColor("#3b4261")); 
        p.end();
        emptyIconLabel->setPixmap(emptyPix);
    }
    emptyLayout->addWidget(emptyIconLabel);
    
    QLabel *emptyTextLabel = new QLabel("No drives mounted.\nGo to the Drives tab to mount one!", m_emptyWidget);
    emptyTextLabel->setAlignment(Qt::AlignCenter);
    emptyTextLabel->setStyleSheet("color: #565f89; font-size: 18px; font-weight: bold; background: transparent;");
    emptyLayout->addWidget(emptyTextLabel);
    
    contentLayout->addWidget(m_emptyWidget, 1);

    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    m_model->setRootPath(m_rootPath);

    m_view = new QListView(contentWidget);
    m_view->setViewMode(QListView::ListMode);
    m_view->setItemDelegate(new BrowseDelegate(this));
    m_view->setSpacing(4);
    m_view->setUniformItemSizes(true);
    
    m_view->setAttribute(Qt::WA_Hover, true);
    m_view->viewport()->setAttribute(Qt::WA_Hover, true);

    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->verticalScrollBar()->setSingleStep(20);
    m_view->setModel(m_model);
    m_view->setRootIndex(m_model->index(m_rootPath));
    m_view->setFrameShape(QFrame::NoFrame);
    
    m_view->setStyleSheet(R"(
        QListView { background: transparent; outline: none; border: none; }
        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
        QScrollBar:handle:vertical { background: #414868; min-height: 40px; border-radius: 4px; }
        QScrollBar:handle:vertical:hover { background: #7aa2f7; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");
    
    connect(m_view, &QListView::doubleClicked, this, &BrowsePage::onItemDoubleClicked);
    
    m_view->viewport()->installEventFilter(this);
    
    contentLayout->addWidget(m_view, 1);
    layout->addWidget(contentWidget, 1);
    checkEmptyState();
}

void BrowsePage::refresh() {
    QString savedPath = m_currentPath; 
    m_view->setModel(nullptr);
    delete m_model;
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    m_model->setRootPath(m_rootPath);
    m_view->setModel(m_model);
    onDirectoryLoaded(savedPath); 
    reload();
}

void BrowsePage::reload() { checkEmptyState(); }

void BrowsePage::checkEmptyState() {
    QDir rootDir(m_rootPath);
    if (m_currentPath == m_rootPath && rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty()) {
        m_view->hide();
        m_emptyWidget->show();
    } else {
        m_emptyWidget->hide();
        m_view->show();
        m_model->setRootPath(m_rootPath);
    }
}

void BrowsePage::onDirectoryLoaded(const QString &path) {
    if (!path.startsWith(m_rootPath)) return; 
    
    m_currentPath = path;
    m_view->setRootIndex(m_model->index(m_currentPath));
    
    if (m_currentPath == m_rootPath) {
        m_backBtn->hide();
        m_pathLabel->setText("Files");
    } else {
        m_backBtn->show();
        QString relative = QDir(m_rootPath).relativeFilePath(m_currentPath);
        m_pathLabel->setText(relative);
    }
}

bool BrowsePage::isMediaFile(const QString &fileName) const {
    QString lower = fileName.toLower();
    QStringList exts = {".mp4", ".mkv", ".avi", ".mov", ".webm", ".flv", ".wmv", ".m4v"};
    for (const QString &ext : exts) {
        if (lower.endsWith(ext)) return true;
    }
    return false;
}

void BrowsePage::onPlayButtonClicked(const QString &path, const QString &title) {
    if (!path.isEmpty()) {
        emit playVideoRequested(path, title, QPixmap());
    }
}

void BrowsePage::onItemDoubleClicked(const QModelIndex &index) {
    QFileInfo info = m_model->fileInfo(index);
    if (info.isDir()) {
        onDirectoryLoaded(info.absoluteFilePath());
    } else {
        if (isMediaFile(info.fileName())) {
            onPlayButtonClicked(info.absoluteFilePath(), info.fileName());
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
        }
    }
}

bool BrowsePage::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_view->viewport() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QModelIndex index = m_view->indexAt(mouseEvent->pos());
            if (index.isValid()) {
                QFileInfo info = m_model->fileInfo(index);
                if (!info.isDir() && isMediaFile(info.fileName())) {
                    QRect rect = m_view->visualRect(index);
                    
                    QRect playButtonRect(rect.right() - 55, rect.top(), 50, rect.height()); 
                    
                    if (playButtonRect.contains(mouseEvent->pos())) {
                        if (event->type() == QEvent::MouseButtonRelease) {
                            onPlayButtonClicked(info.absoluteFilePath(), info.fileName());
                        }
                        return true;
                    }
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}