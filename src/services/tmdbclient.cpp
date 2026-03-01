#include "tmdbclient.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QSettings>
#include <QNetworkDiskCache>
#include <QStandardPaths>

TmdbClient::TmdbClient(QObject *parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/tmdb_images");
    diskCache->setMaximumCacheSize(100 * 1024 * 1024);
    m_manager->setCache(diskCache);
}

void TmdbClient::fetchPoster(const MediaInfo &info, const QString &id) {
    QSettings settings("Kino", "AppConfig");
    QString apiKey = settings.value("tmdb_api_key", "").toString().trimmed();
    if (apiKey.isEmpty()) return;

    QString type = info.isSeries ? "tv" : "movie";
    QUrl url("https://api.themoviedb.org/3/search/" + type);
    
    QUrlQuery query;
    query.addQueryItem("api_key", apiKey);
    query.addQueryItem("query", info.isSeries ? info.seriesName : info.title);
    url.setQuery(query);

    auto processJsonResponse = [this, id](const QByteArray &responseData) {
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isObject()) return;
        
        QJsonArray results = doc.object()["results"].toArray();
        if (results.isEmpty()) return;

        QJsonObject firstResult = results[0].toObject();
        
        emit metadataLoaded(id, firstResult);

        QString posterPath = firstResult["poster_path"].toString();
        if (!posterPath.isEmpty()) {
            if (!posterPath.startsWith("/")) posterPath.prepend("/"); 
            downloadImage(id, "https://image.tmdb.org/t/p/w500" + posterPath, false);
        }

        QString backdropPath = firstResult["backdrop_path"].toString();
        if (!backdropPath.isEmpty()) {
            if (!backdropPath.startsWith("/")) backdropPath.prepend("/"); 
            downloadImage(id, "https://image.tmdb.org/t/p/original" + backdropPath, true);
        }
    };

    if (m_manager->cache()) {
        QIODevice *cachedData = m_manager->cache()->data(url);
        if (cachedData) {
            QByteArray data = cachedData->readAll();
            delete cachedData; 
            processJsonResponse(data);
            return; 
        }
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, processJsonResponse]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        processJsonResponse(reply->readAll());
    });
}

void TmdbClient::downloadImage(const QString &id, const QString &urlString, bool isBackdrop) {
    QUrl url(urlString);
    if (m_manager->cache()) {
        QIODevice *cachedData = m_manager->cache()->data(url);
        if (cachedData) {
            QPixmap pixmap;
            if (pixmap.loadFromData(cachedData->readAll())) {
                if (isBackdrop) emit backdropLoaded(id, pixmap);
                else emit posterLoaded(id, pixmap);
            }
            delete cachedData;
            return; 
        }
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, id, isBackdrop]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            if (isBackdrop) emit backdropLoaded(id, pixmap);
            else emit posterLoaded(id, pixmap);
        }
    });
}