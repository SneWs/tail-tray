#include "MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>

#if defined(WINDOWS_BUILD)
#include <QStyleFactory>
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("grenangen");
    QCoreApplication::setOrganizationDomain("grenangen.se");
    QCoreApplication::setApplicationName("Tail Tray");

    QApplication a(argc, argv);
#if defined(WINDOWS_BUILD)
    a.setStyle(QStyleFactory::create("Fusion"));
#endif
    QApplication::setQuitOnLastWindowClosed(false);

    QTranslator translator;

    QLocale locale = QLocale::system();
    QDir dir = QApplication::applicationDirPath();
    auto didLoad = translator.load(locale, "tail_tray", "_", dir.absolutePath());
    if (didLoad)
        QCoreApplication::installTranslator(&translator);
    else {
        // Load from default install din
#if !defined(WINDOWS_BUILD)
        // /usr/local/lib/tail-tray
        dir = QDir("/usr/local/lib/tail-tray");
        didLoad = translator.load(locale, "tail_tray", "_", dir.absolutePath());
        if (didLoad) {
            QCoreApplication::installTranslator(&translator);
        }
#endif
    }

    qDebug() << "Tail tray loaded translations from" << dir.absolutePath();

    MainWindow w;
    w.hide();

    return a.exec();
}
