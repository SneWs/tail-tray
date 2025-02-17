#include "AdvertiseRoutesDlg.h"
#include "ui_AdvertiseRoutesDlg.h"

#include <QRegularExpression>
#include <QHostAddress>
#include <QString>
#include <QMessageBox>
#include <QDebug>

namespace {
    bool isValidIPv4WithSubnet(const QString& input) {
        // Regular expression for matching "xxx.xxx.xxx.xxx/yy"
        static QRegularExpression regex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})/(\d{1,2})$)");
        QRegularExpressionMatch match = regex.match(input);
    
        if (!match.hasMatch()) {
            return false; // Format is incorrect
        }
    
        // Extract IP parts
        QStringList ipParts = input.split('/').first().split('.');
        int subnet = input.split('/').last().toInt();
    
        // Validate each octet (0-255)
        for (const QString &part : ipParts) {
            int value = part.toInt();
            if (value < 0 || value > 255) {
                return false;
            }
        }
    
        // Validate subnet mask (0-32)
        if (subnet < 0 || subnet > 32) {
            return false;
        }
    
        // Validate the IP address using QHostAddress
        QHostAddress ipAddress(input.split('/').first());
        if (ipAddress.protocol() != QAbstractSocket::IPv4Protocol) {
            return false;
        }
    
        return true;
    }
}

AdvertiseRoutesDlg::AdvertiseRoutesDlg(const QList<QString>& routes, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AdvertiseRoutesDlg)
    , definedRoutes()
{
    ui->setupUi(this);

    QObject::connect(ui->btnClose, &QPushButton::clicked, this, &AdvertiseRoutesDlg::validateAndClose);
    QObject::connect(ui->btnAddRoute, &QPushButton::clicked, this, &AdvertiseRoutesDlg::validateAndAddRoute);
    QObject::connect(ui->btnRemoveSelected, &QPushButton::clicked, this, &AdvertiseRoutesDlg::removeSelectedRoutes);

    // Sync with UI
    for (const QString& route : routes) {
        ui->lstRoutes->addItem(route);
    }

    ui->txtRoute->setText("0.0.0.0/0");
}

AdvertiseRoutesDlg::~AdvertiseRoutesDlg()
{
    delete ui;
}

void AdvertiseRoutesDlg::validateAndClose() {
    // Copy over what we have from UI
    definedRoutes.clear();
    for (int i = 0; i < ui->lstRoutes->count(); ++i) {
        definedRoutes.push_back(ui->lstRoutes->item(i)->text());
    }

    accept();
    close();
}

void AdvertiseRoutesDlg::validateAndAddRoute() {
    QString errors{};

    QString route = ui->txtRoute->text();
    if (route.isEmpty()) {
        errors = "Route cannot be empty";
    }

    if (!isValidIPv4WithSubnet(route)) {
        errors = "Invalid route format. Expected: xxx.xxx.xxx.xxx/yy";
    }

    if (!errors.isEmpty()) {
        qDebug() << "Error: " << errors;
        QMessageBox::warning(this, "Validation Error", errors, QMessageBox::Ok);
        return;
    }

    ui->lstRoutes->addItem(route);    
    ui->txtRoute->setText("0.0.0.0/0");
}

void AdvertiseRoutesDlg::removeSelectedRoutes() {
    for (int i = ui->lstRoutes->count() - 1; i >= 0; --i) {
        if (!ui->lstRoutes->item(i)->isSelected()) {
            continue;
        }

        delete ui->lstRoutes->takeItem(i);
    }
}