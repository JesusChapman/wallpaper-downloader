#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QScrollBar>
#include <QDebug>
#include <QDesktopServices>
#include <QStyleFactory>
#include <QFile>
#include <utility>
#include <QStyle>

// --- WINDOWS SPECIFIC ---
#ifdef Q_OS_WIN
#include <Windows.h>
#include <WinUser.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#endif
// ------------------------

// --- LINUX / KDE SPECIFIC ---
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <KWindowEffects>
#include <KConfig>
#include <KConfigGroup>
#endif
// ----------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , api(new WallhavenApi(this))
    , currentPage(1)
    , isLoading(false)
    , currentQuery("")
    , currentSorting("toplist")
    , currentCategories("111")
    , themeMode(0)
    , backdropMode(2)
{
    ui->setupUi(this);
    ui->statusbar->setSizeGripEnabled(false);

    this->setAttribute(Qt::WA_TranslucentBackground);

    connect(api, &WallhavenApi::wallpapersReceived, this, &MainWindow::onWallpapersReceived);
    connect(api, &WallhavenApi::thumbnailDownloaded, this, &MainWindow::onThumbnailDownloaded);
    connect(api, &WallhavenApi::wallpaperDownloaded, this, &MainWindow::onWallpaperDownloaded);
    connect(api, &WallhavenApi::downloadProgress, this, &MainWindow::onDownloadProgress);
    connect(api, &WallhavenApi::errorOccurred, this, &MainWindow::onApiError);

    loadSettings();
    setupUI();

    fetchWallpapers();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateWindowStyle();
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_SETTINGCHANGE && msg->wParam == 0) {
            if (themeMode == 0) {
                updateWindowStyle();
            }
        }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::loadSettings()
{
    QSettings settings("wallpaper_downloader_app", "Qt Application");
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/backgrounds";

    downloadPath = settings.value("downloadPath", defaultPath).toString();
    themeMode = settings.value("themeMode", 0).toInt();
    backdropMode = settings.value("backdropMode", 2).toInt();
}

void MainWindow::saveSettings()
{
    QSettings settings("wallpaper_downloader_app", "Qt Application");
    settings.setValue("downloadPath", downloadPath);
    settings.setValue("themeMode", themeMode);
    settings.setValue("backdropMode", backdropMode);
}

bool MainWindow::isSystemInDarkMode()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    int appsUseLightTheme = settings.value("AppsUseLightTheme", 1).toInt();
    return (appsUseLightTheme == 0);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    KConfig config("kdeglobals");
    KConfigGroup group = config.group("Colors:Window");
    QColor windowColor = group.readEntry("BackgroundNormal", QColor(239, 239, 239));
    return (windowColor.lightness() < 128);
#else
    return false;
#endif
}

QColor MainWindow::getSystemAccentColor()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM", QSettings::NativeFormat);
    QVariant variant = settings.value("AccentColor");

    if (variant.isValid()) {
        unsigned int value = variant.toUInt();
        int r = value & 0xFF;
        int g = (value >> 8) & 0xFF;
        int b = (value >> 16) & 0xFF;
        return QColor(r, g, b);
    }
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    KConfig config("kdeglobals");
    KConfigGroup group = config.group("General");
    QColor accent = group.readEntry("AccentColor", QColor());
    if (accent.isValid()) {
        return accent;
    }
    KConfigGroup selGroup = config.group("Colors:Selection");
    return selGroup.readEntry("BackgroundNormal", QColor(61, 174, 233));
#endif
    return QColor(0, 120, 215);
}

void MainWindow::updateWindowStyle()
{
    bool useDark = false;
    if (themeMode == 0) {
        useDark = isSystemInDarkMode();
    } else {
        useDark = (themeMode == 2);
    }

#ifdef Q_OS_WIN
    HWND hwnd = (HWND)this->winId();
    QOperatingSystemVersion version = QOperatingSystemVersion::current();
    int build = version.microVersion();

    if (build >= 22000) {
        int dwmIsDark = useDark ? 1 : 0;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dwmIsDark, sizeof(dwmIsDark));

        MARGINS margins = {-1};
        DwmExtendFrameIntoClientArea(hwnd, &margins);

        if (build >= 22621) {
            int backdrop = backdropMode;
            DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
        } else {
            int micaValue = 1;
            DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &micaValue, sizeof(micaValue));
        }
    }
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (backdropMode == 3) {
        KWindowEffects::enableBlurBehind(this->windowHandle(), true);
    } else {
        KWindowEffects::enableBlurBehind(this->windowHandle(), false);
    }
#endif

    applyStyles(useDark);

    // CORRECCIÓN: Forzar repintado profundo de widgets
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
}

void MainWindow::applyStyles(bool isDark)
{
    QColor accent = getSystemAccentColor();
    QColor accentLight = accent.lighter(130);
    QColor accentDark = accent.darker(110);

    QString colAccent = accent.name();
    QString colAccentLight = accentLight.name();
    QString colAccentDark = accentDark.name();
    QString colAccentAlpha = QString("rgba(%1, %2, %3, 180)")
                                 .arg(QString::number(accent.red()),
                                      QString::number(accent.green()),
                                      QString::number(accent.blue()));

    QPalette palette;
    if (isDark) {
        palette.setColor(QPalette::Window, QColor(30, 30, 30));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(45, 45, 45));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::Link, accentLight);
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, Qt::black);
    } else {
        palette.setColor(QPalette::Window, QColor(240, 240, 240));
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, QColor(233, 231, 227));
        palette.setColor(QPalette::ToolTipBase, Qt::black);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(240, 240, 240));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::Link, accent);
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, Qt::white);
    }
    qApp->setPalette(palette);

    QFile file(":/style.qss");
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "No se pudo abrir style.qss";
        return;
    }
    QString styleSheet = QLatin1String(file.readAll());
    file.close();

    if (isDark) {
        styleSheet.replace("%TEXT_COLOR%", "#eeeeee");
        styleSheet.replace("%TEXT_COLOR_ALT%", "white");
        styleSheet.replace("%HOVER_BG%", "rgba(255, 255, 255, 20)");
        styleSheet.replace("%MENU_BG%", "rgba(30, 30, 30, 230)");
        styleSheet.replace("%BORDER_COLOR%", "rgba(255, 255, 255, 20)");
        styleSheet.replace("%DIALOG_BG%", "#1e1e1e");
        styleSheet.replace("%PROGRESS_BG%", "rgba(255, 255, 255, 20)");
        styleSheet.replace("%ACCENT_COLOR%", colAccent);
        styleSheet.replace("%ACCENT_LIGHT%", colAccentLight);
        styleSheet.replace("%ACCENT_DARK%", colAccentDark);
        styleSheet.replace("%ACCENT_ALPHA%", colAccentAlpha);

        styleSheet.replace("%SIDEBAR_BG%", "rgba(43, 43, 43, 75)");
        styleSheet.replace("%SIDEBAR_BORDER%", "rgba(62, 62, 62, 100)");
        styleSheet.replace("%BTN_TEXT%", "#cccccc");
        styleSheet.replace("%LABEL_COLOR%", "#ddd");
        styleSheet.replace("%INPUT_BG%", "rgba(0, 0, 0, 50)");
        styleSheet.replace("%INPUT_BORDER%", "rgba(80, 80, 80, 100)");

        styleSheet.replace("%ITEM_BG%", "rgba(45, 45, 45, 75)");
        styleSheet.replace("%ITEM_BORDER%", "rgba(255, 255, 255, 15)");
        styleSheet.replace("%ITEM_HOVER_BG%", "rgba(60, 60, 60, 120)");
    } else {
        styleSheet.replace("%TEXT_COLOR%", "#111111");
        styleSheet.replace("%TEXT_COLOR_ALT%", "black");
        styleSheet.replace("%HOVER_BG%", "rgba(0, 0, 0, 10)");
        styleSheet.replace("%MENU_BG%", "rgba(245, 245, 245, 230)");
        styleSheet.replace("%BORDER_COLOR%", "rgba(0, 0, 0, 20)");
        styleSheet.replace("%DIALOG_BG%", "#f0f0f0");
        styleSheet.replace("%PROGRESS_BG%", "rgba(0, 0, 0, 20)");
        styleSheet.replace("%ACCENT_COLOR%", colAccent);
        styleSheet.replace("%ACCENT_LIGHT%", colAccentLight);
        styleSheet.replace("%ACCENT_DARK%", colAccentDark);
        styleSheet.replace("%ACCENT_ALPHA%", colAccentAlpha);

        styleSheet.replace("%SIDEBAR_BG%", "rgba(255, 255, 255, 80)");
        styleSheet.replace("%SIDEBAR_BORDER%", "rgba(0, 0, 0, 20)");
        styleSheet.replace("%BTN_TEXT%", "#222");
        styleSheet.replace("%LABEL_COLOR%", "#333");
        styleSheet.replace("%INPUT_BG%", "rgba(255, 255, 255, 120)");
        styleSheet.replace("%INPUT_BORDER%", "rgba(0, 0, 0, 30)");

        styleSheet.replace("%ITEM_BG%", "rgba(255, 255, 255, 80)");
        styleSheet.replace("%ITEM_BORDER%", "rgba(0, 0, 0, 10)");
        styleSheet.replace("%ITEM_HOVER_BG%", "rgba(255, 255, 255, 150)");
    }

    qApp->setStyleSheet(styleSheet);
}

void MainWindow::setupUI()
{
    ui->listWidget->setViewMode(QListWidget::IconMode);
    ui->listWidget->setIconSize(QSize(200, 150));
    ui->listWidget->setResizeMode(QListWidget::Adjust);
    ui->listWidget->setSpacing(10);
    ui->listWidget->setMovement(QListWidget::Static);

    ui->progressBar->setVisible(false);
    ui->progressBar->setValue(0);

    ui->btnSearchAction->setCheckable(true);

    connect(ui->listWidget->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrollValueChanged);
    connect(ui->listWidget, &QListWidget::itemClicked, this, &MainWindow::onListItemClicked);
    connect(ui->inputSearch, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(ui->btnSearchAction, &QPushButton::clicked, this, &MainWindow::onSearchClicked);

    connect(ui->btnToplist, &QPushButton::clicked, this, &MainWindow::onCategoryButtonClicked);
    connect(ui->btnAnime, &QPushButton::clicked, this, &MainWindow::onCategoryButtonClicked);
    connect(ui->btnNature, &QPushButton::clicked, this, &MainWindow::onCategoryButtonClicked);
    connect(ui->btnPeople, &QPushButton::clicked, this, &MainWindow::onCategoryButtonClicked);
    connect(ui->btnArt, &QPushButton::clicked, this, &MainWindow::onCategoryButtonClicked);
    connect(ui->btnSciFi, &QPushButton::clicked, this, &MainWindow::onCategoryButtonClicked);

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onActionSettingsTriggered);
    connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::onActionAboutQtTriggered);
    connect(ui->actionAboutApp, &QAction::triggered, this, &MainWindow::onActionAboutAppTriggered);
    connect(ui->actionOpenDownloadsFolder, &QAction::triggered, this, &MainWindow::onActionOpenFolderTriggered);
}

void MainWindow::onActionSettingsTriggered()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Application Settings"));
    dialog.setMinimumWidth(500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    layout->addWidget(new QLabel(tr("Application Theme:"), &dialog));
    QComboBox *comboTheme = new QComboBox(&dialog);
    comboTheme->addItem(tr("Follow System"), 0);
    comboTheme->addItem(tr("Light"), 1);
    comboTheme->addItem(tr("Dark"), 2);
    comboTheme->setCurrentIndex(themeMode);
    layout->addWidget(comboTheme);

    layout->addWidget(new QLabel(tr("Background Effect (Requires partial restart):"), &dialog));
    QComboBox *comboEffect = new QComboBox(&dialog);
    comboEffect->addItem(tr("Mica (Opaque, Performance)"), 2);
    comboEffect->addItem(tr("Acrylic (Translucent, Blur)"), 3);
    comboEffect->setCurrentIndex(backdropMode == 3 ? 1 : 0);
    layout->addWidget(comboEffect);

    layout->addWidget(new QLabel(tr("Destination Folder:"), &dialog));
    QHBoxLayout *hLayout = new QHBoxLayout();
    QLineEdit *pathEdit = new QLineEdit(downloadPath, &dialog);
    QPushButton *browseBtn = new QPushButton("...", &dialog);
    browseBtn->setFixedWidth(40);
    hLayout->addWidget(pathEdit);
    hLayout->addWidget(browseBtn);
    layout->addLayout(hLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    connect(browseBtn, &QPushButton::clicked, [this, pathEdit, &dialog]() {
        QString dir = QFileDialog::getExistingDirectory(&dialog, tr("Select Folder"), pathEdit->text(), QFileDialog::ShowDirsOnly);
        if (!dir.isEmpty()) pathEdit->setText(dir);
    });

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString newPath = pathEdit->text();
        if (!newPath.isEmpty()) downloadPath = newPath;

        themeMode = comboTheme->currentData().toInt();
        backdropMode = comboEffect->currentData().toInt();

        saveSettings();

        // Actualizar estilo inmediatamente
        updateWindowStyle();

        ui->statusbar->showMessage(tr("Settings updated"), 3000);
    }
}

void MainWindow::onActionOpenFolderTriggered()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(downloadPath));
}

void MainWindow::onActionAboutQtTriggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::onActionAboutAppTriggered()
{
    QMessageBox::about(this, tr("About Wallpaper downloader"),
                       tr("<h3>Wallpaper downloader v1.0-dev-preview</h3>"
                          "<p>Created by: Jesus Chapman<br>"
                          "A simple app for download wallpapers using wallhaven api and Qt Tecnologies</p>"));
}

void MainWindow::resetAndSearch(const QString &query, const QString &sorting, const QString &categories)
{
    currentQuery = query;
    currentSorting = sorting;
    currentCategories = categories;
    currentPage = 1;
    ui->listWidget->clear();
    ui->listWidget->verticalScrollBar()->setValue(0);
    fetchWallpapers();
}

void MainWindow::onSearchClicked()
{
    QString text = ui->inputSearch->text().trimmed();
    if (text.isEmpty()) return;

    ui->btnToplist->setAutoExclusive(false);
    ui->btnAnime->setAutoExclusive(false);
    ui->btnNature->setAutoExclusive(false);
    ui->btnPeople->setAutoExclusive(false);
    ui->btnArt->setAutoExclusive(false);
    ui->btnSciFi->setAutoExclusive(false);

    ui->btnToplist->setChecked(false);
    ui->btnAnime->setChecked(false);
    ui->btnNature->setChecked(false);
    ui->btnPeople->setChecked(false);
    ui->btnArt->setChecked(false);
    ui->btnSciFi->setChecked(false);

    ui->btnToplist->setAutoExclusive(true);
    ui->btnAnime->setAutoExclusive(true);
    ui->btnNature->setAutoExclusive(true);
    ui->btnPeople->setAutoExclusive(true);
    ui->btnArt->setAutoExclusive(true);
    ui->btnSciFi->setAutoExclusive(true);

    ui->btnSearchAction->setChecked(true);
    resetAndSearch(text, "relevance", "111");
}

void MainWindow::onCategoryButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    ui->btnSearchAction->setChecked(false);
    QString name = btn->objectName();
    if (name == "btnToplist") resetAndSearch("", "toplist", "111");
    else if (name == "btnAnime") resetAndSearch("", "relevance", "010");
    else if (name == "btnPeople") resetAndSearch("", "relevance", "001");
    else if (name == "btnNature") resetAndSearch("nature", "relevance", "100");
    else if (name == "btnArt") resetAndSearch("art", "relevance", "100");
    else if (name == "btnSciFi") resetAndSearch("scifi", "relevance", "100");
}

void MainWindow::fetchWallpapers()
{
    if (isLoading) return;
    isLoading = true;
    api->searchWallpapers(currentQuery, currentSorting, currentCategories, currentPage);
}

void MainWindow::onWallpapersReceived(const QJsonArray &dataArray)
{
    if (dataArray.isEmpty() && currentPage == 1)
        QMessageBox::information(this, tr("Info"), tr("No results found."));

    for (const QJsonValue &val : std::as_const(dataArray)) {
        QJsonObject imgObj = val.toObject();
        QString thumbUrl = imgObj["thumbs"].toObject()["small"].toString();
        QString fullUrl = imgObj["path"].toString();
        QString imgId = imgObj["id"].toString();
        QString resolution = imgObj["resolution"].toString();

        api->setProperty("_temp_fullUrl", fullUrl);
        api->downloadThumbnail(thumbUrl, imgId, resolution);
    }
    currentPage++;
    isLoading = false;
}

void MainWindow::onThumbnailDownloaded(const QString &id, const QString &resolution, const QByteArray &data, const QString &fullUrl)
{
    QPixmap pixmap;
    pixmap.loadFromData(data);
    QListWidgetItem *item = new QListWidgetItem();
    item->setIcon(QIcon(pixmap));
    item->setText(resolution.isEmpty() ? "Unknown" : resolution);
    item->setData(Qt::UserRole, fullUrl);
    ui->listWidget->addItem(item);
}

void MainWindow::onApiError(const QString &errorString)
{
    isLoading = false;
    ui->statusbar->showMessage("API Error: " + errorString, 5000);
}

void MainWindow::onScrollValueChanged(int value)
{
    QScrollBar *bar = ui->listWidget->verticalScrollBar();
    if (value > bar->maximum() * 0.9 && !isLoading) fetchWallpapers();
}

void MainWindow::onListItemClicked(QListWidgetItem *item)
{
    QString fullUrl = item->data(Qt::UserRole).toString();
    QString fileName = QFileInfo(fullUrl).fileName();

    ui->statusbar->showMessage(tr("Starting download: ") + fileName);

    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);

    api->downloadWallpaper(fullUrl, fileName);
}

void MainWindow::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        ui->progressBar->setValue(percentage);
    }
}

void MainWindow::onWallpaperDownloaded(const QString &fileName, const QByteArray &data)
{
    ui->progressBar->setVisible(false);

    if (data.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Download failed."));
        return;
    }

    QString saveDir = downloadPath;
    QString fullPath = saveDir + "/" + fileName;
    QDir dir; if (!dir.exists(saveDir)) dir.mkpath(saveDir);
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();

        ui->statusbar->showMessage(tr("Saved: ") + fullPath, 3000);
        showPreviewDialog(fullPath);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Download failed."));
    }
}

void MainWindow::showPreviewDialog(const QString &filePath)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Preview"));
    dialog->setMinimumWidth(600);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QLabel *imageLabel = new QLabel(dialog);
    QPixmap pixmap(filePath);
    if (!pixmap.isNull()) {
        imageLabel->setPixmap(pixmap.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imageLabel->setAlignment(Qt::AlignCenter);
    }
    layout->addWidget(imageLabel);
    QHBoxLayout *btnLayout = new QHBoxLayout();

    QPushButton *btnOpen = new QPushButton(tr("Open Folder"), dialog);
    QPushButton *btnClose = new QPushButton(tr("Close"), dialog);

    btnLayout->addStretch(); btnLayout->addWidget(btnOpen); btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);
    connect(btnClose, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(btnOpen, &QPushButton::clicked, [filePath]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
    });
    dialog->exec();
}
