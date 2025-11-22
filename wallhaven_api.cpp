#include "wallhaven_api.h"
#include <QUrl>
#include <QDebug>

WallhavenApi::WallhavenApi(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{
}

void WallhavenApi::searchWallpapers(const QString &query, const QString &sorting, const QString &categories, int page)
{
    QString urlStr = QString("https://wallhaven.cc/api/v1/search?categories=%1&purity=100&sorting=%2&order=desc&page=%3")
                         .arg(categories, sorting, QString::number(page));
    if (!query.isEmpty()) {
        urlStr += "&q=" + query;
    }

    QNetworkRequest request((QUrl(urlStr)));
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        this->onSearchFinished(reply);
    });
}

void WallhavenApi::onSearchFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();

    emit wallpapersReceived(dataArray);
    reply->deleteLater();
}

void WallhavenApi::downloadThumbnail(const QString &url, const QString &id, const QString &resolution)
{
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = manager->get(request);
    
    reply->setProperty("id", id);
    reply->setProperty("resolution", resolution);
    
    QString fullUrl = this->property("_temp_fullUrl").toString();
    reply->setProperty("fullUrl", fullUrl);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        this->onThumbnailFinished(reply);
    });
}

void WallhavenApi::onThumbnailFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QString id = reply->property("id").toString();
        QString res = reply->property("resolution").toString();
        QString fullUrl = reply->property("fullUrl").toString();

        emit thumbnailDownloaded(id, res, data, fullUrl);
    }
    reply->deleteLater();
}

void WallhavenApi::downloadWallpaper(const QString &url, const QString &fileName)
{
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("fileName", fileName);

    connect(reply, &QNetworkReply::downloadProgress, this, &WallhavenApi::downloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        this->onWallpaperFinished(reply);
    });
}

void WallhavenApi::onWallpaperFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QString fileName = reply->property("fileName").toString();
        QByteArray data = reply->readAll();
        emit wallpaperDownloaded(fileName, data);
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}
