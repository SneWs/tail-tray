#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("grenangen");
    QCoreApplication::setOrganizationDomain("grenangen.se");
    QCoreApplication::setApplicationName("tail-tray");

    QApplication a(argc, argv);
    MainWindow w;
    w.hide();

    return a.exec();
}
