#include "MainWindow.h"
#include "SingleApplicationImpl.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>

int main(int argc, char** argv) {
    QCoreApplication::setOrganizationName("grenangen");
    QCoreApplication::setOrganizationDomain("grenangen.se");
    QCoreApplication::setApplicationName("tail-tray");

    SingleApplicationImpl a(argc, argv);
    if (!a.claimInstance()) {
        qDebug() << "Secondary instance not allowed, will quite this instance";
        return -1;
    }

    QApplication::setQuitOnLastWindowClosed(false);

    // Locale setup
    QLocale locale = QLocale::C;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--locale") == 0) {
            locale = QLocale(argv[++i]);
        }
    }

    // No valid/or none given for locale at startup
    if (locale == QLocale::C) {
        locale = QLocale::system();
    }
    else {
        // Set as default
        QLocale::setDefault(locale);
    }

    qDebug() << "Will use locale: " << locale.name() << "for application";

    // Setup and load translations
    QString translationDirRoot = QApplication::applicationDirPath();
#if defined(TRANSLATIONS_DIR)
    qDebug() << "Using TRANSLATION_DIR from cmake compile definition!";
    translationDirRoot = TRANSLATIONS_DIR;
#endif

    qDebug() << "Translation dir: " << translationDirRoot;

    QTranslator translator;

    // Try to load
    auto didLoad = translator.load(locale, "tail_tray", "_", translationDirRoot);
    if (didLoad)
        QCoreApplication::installTranslator(&translator);
    else {
        // Load from fallback, eg same path as the binary is located at
        didLoad = translator.load(locale, "tail_tray", "_",
            QApplication::applicationDirPath());

        if (didLoad) {
            QCoreApplication::installTranslator(&translator);
        }
        else {
            qDebug() << "Failed to load translations from all known locations";
        }
    }

    MainWindow w;
    w.hide();

    auto ec = a.exec();
    return ec;
}
