#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QJsonObject>
#include "mediaparser.h"

class TmdbClient : public QObject
{
    Q_OBJECT
public:
    explicit TmdbClient(QObject *parent = nullptr);
    void fetchPoster(const MediaInfo &info, const QString &id);

signals:
    void posterLoaded(const QString &id, const QPixmap &poster);
    void backdropLoaded(const QString &id, const QPixmap &backdrop);
    void metadataLoaded(const QString &id, const QJsonObject &metadata);

private:
    QNetworkAccessManager *m_manager;
    void downloadImage(const QString &id, const QString &urlString, bool isBackdrop);
};