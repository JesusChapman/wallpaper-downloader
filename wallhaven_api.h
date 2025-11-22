#ifndef WALLHAVEN_API_H
#define WALLHAVEN_API_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class WallhavenApi : public QObject
{
    Q_OBJECT
public:
    explicit WallhavenApi(QObject *parent = nullptr);

    // Funciones públicas para iniciar acciones
    void searchWallpapers(const QString &query, const QString &sorting, const QString &categories, int page);
    void downloadThumbnail(const QString &url, const QString &id, const QString &resolution);
    void downloadWallpaper(const QString &url, const QString &fileName);

signals:
    // Señales para devolver datos a la UI
    void wallpapersReceived(const QJsonArray &data);
    void thumbnailDownloaded(const QString &id, const QString &resolution, const QByteArray &data, const QString &fullUrl);
    void wallpaperDownloaded(const QString &fileName, const QByteArray &data);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void errorOccurred(const QString &errorString);

private slots:
    void onSearchFinished(QNetworkReply *reply);
    void onThumbnailFinished(QNetworkReply *reply);
    void onWallpaperFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif // WALLHAVEN_API_H
