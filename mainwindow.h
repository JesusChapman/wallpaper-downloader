#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QSettings>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QOperatingSystemVersion>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QColor>

// api de wallhaven
#include "wallhaven_api.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    // Slots conectados a WallhavenApi
    void onWallpapersReceived(const QJsonArray &data);
    void onThumbnailDownloaded(const QString &id, const QString &resolution, const QByteArray &data, const QString &fullUrl);
    void onWallpaperDownloaded(const QString &fileName, const QByteArray &data);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onApiError(const QString &errorString);

    // UI Slots
    void onScrollValueChanged(int value);
    void onListItemClicked(QListWidgetItem *item);
    void onSearchClicked();
    void onCategoryButtonClicked();
    void onActionSettingsTriggered();
    void onActionAboutQtTriggered();
    void onActionAboutAppTriggered();
    void onActionOpenFolderTriggered();

private:
    Ui::MainWindow *ui;
    WallhavenApi *api; // Reemplaza a QNetworkAccessManager

    int currentPage;
    bool isLoading;
    QString currentQuery;
    QString currentSorting;
    QString currentCategories;

    QString downloadPath;
    int themeMode;
    int backdropMode;

    void fetchWallpapers();
    void setupUI();
    void showPreviewDialog(const QString &filePath);
    void resetAndSearch(const QString &query, const QString &sorting, const QString &categories);

    void loadSettings();
    void saveSettings();

    void updateWindowStyle();
    void applyStyles(bool isDark);
    bool isSystemInDarkMode();

    QColor getSystemAccentColor();
};

#endif // MAINWINDOW_H
