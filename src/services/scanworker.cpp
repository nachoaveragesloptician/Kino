#include "scanworker.h"
#include <QThread>
#include <QSettings>
#include <QFileInfo>

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

        QString driveName = QFileInfo(mount).fileName();

        QSettings settings("Kino", "DriveFilters");
        QStringList rawWhite = settings.value(driveName + "_whitelist", "").toString().split("\n", Qt::SkipEmptyParts);
        QStringList rawBlack = settings.value(driveName + "_blacklist", "").toString().split("\n", Qt::SkipEmptyParts);

        QStringList whitelist, blacklist;
        for (QString w : rawWhite) {
            w = w.trimmed().toLower();
            while (w.startsWith("/")) w.remove(0, 1);
            if (w.isEmpty()) continue;
            if (!w.endsWith("/")) w += "/";
            whitelist.append(w);
        }
        for (QString b : rawBlack) {
            b = b.trimmed().toLower();
            while (b.startsWith("/")) b.remove(0, 1);
            if (b.isEmpty()) continue;
            if (!b.endsWith("/")) b += "/";
            blacklist.append(b);
        }

        QDir dir(mount);
        if (dir.exists()) {
            scanRecursive(dir, mount, buffer, whitelist, blacklist);
        }
    }

    if (!buffer.isEmpty()) {
        emit batchFound(buffer);
    }
    emit finished();
}

void ScanWorker::scanRecursive(const QDir &dir, const QString &rootMount, QList<VideoFile> &buffer, const QStringList &whitelist, const QStringList &blacklist) {
    {
        QMutexLocker locker(&m_mutex);
        if (m_stopRequested) return;
    }

    QFileInfoList list = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden);
    QStringList allowed = { "mp4", "mkv", "avi", "mov", "webm", "flv", "wmv", "m4v" };

    for (const QFileInfo &info : list) {
        QString relativePath = QDir(rootMount).relativeFilePath(info.filePath()).toLower();

        if (info.isDir()) {
            QString dirPath = relativePath + "/"; 

            bool isBlacklisted = false;
            for (const QString &b : blacklist) {
                if (dirPath.startsWith(b)) {
                    isBlacklisted = true;
                    break;
                }
            }
            if (isBlacklisted) continue;

            if (!whitelist.isEmpty()) {
                bool allowedToEnter = false;
                for (const QString &w : whitelist) {
                    if (dirPath.startsWith(w) || w.startsWith(dirPath)) {
                        allowedToEnter = true;
                        break;
                    }
                }
                if (!allowedToEnter) continue; 
            }

            scanRecursive(QDir(info.filePath()), rootMount, buffer, whitelist, blacklist);
        } 
        else if (info.isFile()) {
            bool isFileBlacklisted = false;
            for (const QString &b : blacklist) {
                if (relativePath.startsWith(b)) {
                    isFileBlacklisted = true;
                    break;
                }
            }
            if (isFileBlacklisted) continue;

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