#pragma once

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QLineEdit>
#include <QThread>
#include <QSet>
#include <QMap>
#include <QList>
#include <QPushButton>
#include <QResizeEvent>
#include <QPixmap>
#include <QVariant>
#include <QMenu>
#include <QAction>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>

#include "scanworker.h"
#include "toast.h"
#include "tmdbclient.h"

enum MediaRole {
    FilePathRole = Qt::UserRole + 1,
    IsStackRole,
    IsDefaultIconRole,
    SubtitleRole,
    BackdropRole,
    TmdbDataRole,
    ProgressRole,
    SeasonRole,
    EpisodeRole
};

class FileView : public QWidget
{
    Q_OBJECT

public:
    explicit FileView(QWidget *parent = nullptr);
    ~FileView();
    void setMounts(const QStringList &mountPaths);

public slots:
    void startScan(const QStringList &targets = QStringList()); 
    void onBatchFound(const QList<VideoFile> &files);
    void onScanFinished();
    void onPosterLoaded(const QString &path, const QPixmap &poster);
    void onBackdropLoaded(const QString &path, const QPixmap &backdrop);
    void onMetadataLoaded(const QString &path, const QJsonObject &metadata);
    void onItemClicked(const QModelIndex &index);
    void onBackClicked();
    void showRescanMenu();
    void onPlayButtonClicked(); 
    void refreshMetadata();

signals:
    void playVideoRequested(const QString &filePath, const QString &title, const QPixmap &banner);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void stopScan();
    void loadFromCache();
    void appendToCache(const QList<VideoFile> &files);
    void updateGridSize(); 
    void setupSidebar();
    void updateSidebar(const QModelIndex &index);
    void clearSidebar();
    
    void loadMediaInfoCache();
    void saveMediaInfoCache();
    void requestMediaInfo(const QString &path, const QString &filename);
    void executeMediaInfoProbe(); 
    
    QString parseMediaInfoText(const QByteArray &output, const QString &path, const QString &filename);
    
    QString formatMediaQuality(const QString &filename);
    QString formatExtendedMediaInfo(const QString &filename);

    QWidget *m_mainContentWidget;
    QLabel *m_titleLabel;
    QLineEdit *m_searchBar;
    QPushButton *m_rescanBtn;
    
    QWidget *m_emptyWidget;
    QLabel *m_emptyIconLabel;
    QLabel *m_emptyTextLabel;

    QListView *m_view;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxy;
    QPushButton *m_backBtn;
    
    QWidget *m_sidebarWidget;
    QLabel *m_sidebarPoster;
    QLabel *m_sidebarTitle;
    QLabel *m_sidebarMeta;
    QLabel *m_sidebarOverview;
    QLabel *m_sidebarFileInfo;
    QPushButton *m_sidebarPlayBtn;
    
    QString m_currentPlayPath;
    QString m_currentPlayTitle;
    QPixmap m_currentBackdrop;

    QThread *m_scanThread = nullptr;
    ScanWorker *m_scanWorker = nullptr;
    TmdbClient *m_tmdb;
    Toast *m_toast;
    
    QStringList m_currentMounts;
    QSet<QString> m_knownPaths;
    QString m_cacheFilePath;
    QString m_mediaInfoCachePath;
    
    QMap<QString, QString> m_mediaInfoCache;
    QTimer *m_probeTimer;
    QString m_pathToProbe;
    QString m_filenameToProbe;

    QMap<QString, QList<VideoFile>> m_mediaGroups; 
    QMap<QString, QStandardItem*> m_groupItems; 
    QList<QStandardItem*> m_mainLibraryItems; 
    bool m_isDetailView = false; 
};