#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;

    // Con esto obtenemos el idioma del sistema ^^
    QLocale systemLocale = QLocale::system();

    // Si el idioma es CUALQUIER tipo de Español (es_ES, es_MX, es_AR, etc.)
    // Si, soy colombiano, generé el ts en Qt creator con es_CO y me dió flojera cambiarlo después.
    if (systemLocale.language() == QLocale::Spanish) {
        // Cargar forzosamente nuestra traducción base (es_CO)
        if (translator.load(":/i18n/wallpaper_downloader_es_CO")) {
            a.installTranslator(&translator);
        }
    }
    else {
        // Para otros idiomas, intentar carga estándar (inglés :p), podrías ayudar a traducir, si quieres ^^
        const QStringList uiLanguages = systemLocale.uiLanguages();
        for (const QString &locale : uiLanguages) {
            const QString baseName = "wallpaper_downloader_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                a.installTranslator(&translator);
                break;
            }
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
