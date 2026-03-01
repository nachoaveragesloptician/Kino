#include "scanworker.h"
#include <QThread>

ScanWorker::ScanWorker(const QStringList &mounts) : m_mounts(mounts) {}

void ScanWorker::requestStop() {
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
}

void ScanWorker::process() {
    QList<VideoFile> buffer;

    for (const QString &mount : m_mounts) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopRequested) break;
        }

        QDir dir(mount);
        if (dir.exists()) {
            scanRecursive(dir, mount, buffer);
        }
    }

    if (!buffer.isEmpty()) {
        emit batchFound(buffer);
    }
    emit finished();
}

void ScanWorker::scanRecursive(const QDir &dir, const QString &rootMount, QList<VideoFile> &buffer) {
    {
        QMutexLocker locker(&m_mutex);
        if (m_stopRequested) return;
    }

    QFileInfoList list = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden);
    QStringList allowed = { "mp4", "mkv", "avi", "mov", "webm", "flv", "wmv", "m4v" };

    for (const QFileInfo &info : list) {
        if (info.isDir()) {
            scanRecursive(QDir(info.filePath()), rootMount, buffer);
        } 
        else if (info.isFile()) {
            if (allowed.contains(info.suffix().toLower())) {
                VideoFile vf;
                vf.name = info.fileName();
                vf.path = info.filePath();
                vf.mountPath = rootMount;
                buffer.append(vf);

                if (buffer.size() >= 10) {
                    emit batchFound(buffer);
                    buffer.clear();
                    QThread::msleep(10); 
                }
            }
        }
    }
}