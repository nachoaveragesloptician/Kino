#pragma once

#include <QObject>
#include <QDir>
#include <QMutex>

struct VideoFile {
    QString name;
    QString path;
    QString mountPath;
};

class ScanWorker : public QObject
{
    Q_OBJECT

public:
    explicit ScanWorker(const QStringList &mounts);
    void requestStop();

public slots:
    void process();

signals:
    void batchFound(QList<VideoFile> files);
    void finished();

private:
    QStringList m_mounts;
    QMutex m_mutex;
    bool m_stopRequested = false;

    void scanRecursive(const QDir &dir, const QString &rootMount, QList<VideoFile> &buffer);
};