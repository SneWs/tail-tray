#ifndef ADVERTISEROUTESDLG_H
#define ADVERTISEROUTESDLG_H

#include <QDialog>

namespace Ui {
class AdvertiseRoutesDlg;
}

class AdvertiseRoutesDlg : public QDialog
{
    Q_OBJECT
public:
    explicit AdvertiseRoutesDlg(const QList<QString>& routes, QWidget* parent = nullptr);
    ~AdvertiseRoutesDlg();

    const QList<QString>& getDefinedRoutes() const { return definedRoutes; }

private slots:
    void validateAndClose();
    void validateAndAddRoute();
    void removeSelectedRoutes();

private:
    QList<QString> definedRoutes;

    Ui::AdvertiseRoutesDlg* ui;
};

#endif // ADVERTISEROUTESDLG_H
