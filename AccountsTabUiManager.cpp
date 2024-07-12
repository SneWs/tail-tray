//
// Created by marcus on 2024-07-11.
//

#include "AccountsTabUiManager.h"

#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>

#include "./ui_MainWindow.h"
#include "MainWindow.h"
#include "models.h"

AccountsTabUiManager::AccountsTabUiManager(Ui::MainWindow* u, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , ui(u)
    , pTailRunner(runner)
{
    connect(ui->btnAdminConsole, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl("https://login.tailscale.com/admin"));
        }
    );

    connect(ui->btnLogout, &QPushButton::clicked, this, [this]() {
            pTailRunner->logout();
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
            wnd->userLoggedOut();
            wnd->hide();
        }
    );
}

AccountsTabUiManager::~AccountsTabUiManager() {
 }

void AccountsTabUiManager::onTailStatusChanged(TailStatus* pTailStatus) {
    if (pTailStatus->user->id <= 0) {
        // Not logged in

        // Hide account details view, nothing to show
        ui->accountDetailsContainer->setVisible(false);
        ui->lstAccounts->clear();
        return;
    }

    // Show account details view
    ui->accountDetailsContainer->setVisible(true);
    ui->lstAccounts->clear();
    auto* pCurrent = new QListWidgetItem(pTailStatus->user->displayName + "\n" +
                                         pTailStatus->user->loginName);
    ui->lstAccounts->addItem(pCurrent);

    pCurrent->setSelected(true);
    pCurrent->setTextAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);

    ui->lblUsername->setText(pTailStatus->user->displayName);
    ui->lblTailnetName->setText(pTailStatus->user->loginName);
    ui->lblEmail->setText(pTailStatus->user->loginName);
    ui->lblStatus->setText(pTailStatus->backendState);
    ui->lblKeyExpiry->setText("");

    // Show the key expiry date in a more human readable format
    const auto now = QDateTime::currentDateTime();
    const auto daysToExpiry = now.daysTo(pTailStatus->self->keyExpiry);
    const auto monthsToExpiry = daysToExpiry / 30;
    if (monthsToExpiry > 0)
        ui->lblKeyExpiry->setText("in " + QString::number(monthsToExpiry) + " months");
    else {
        if (daysToExpiry < 1) {
            ui->lblKeyExpiry->setText("in less then 1 day");
        }
        else {
            ui->lblKeyExpiry->setText("in " + QString::number(daysToExpiry) + " days");
        }
    }

    if (!pTailStatus->user->profilePicUrl.isEmpty()) {
        // ui->lblUsername->setPixmap(QPixmap(pTailStatus->user->profilePicUrl));
    }
}
