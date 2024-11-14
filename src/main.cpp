#include "MainWindow.h"

#include <QApplication>

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

    MainWindow w;
    w.hide();

    return a.exec();
}
