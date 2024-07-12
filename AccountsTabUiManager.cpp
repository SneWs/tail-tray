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

    connect(ui->btnAddAccount, &QPushButton::clicked, this, [this]() {
            pTailRunner->login();
        }
    );

    connect(ui->lstAccounts, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
            // TODO: Get info about selected account
            auto accountId = item->data(Qt::UserRole).toString();
            qDebug() << "Selected account: " << accountId;
        }
    );
}

AccountsTabUiManager::~AccountsTabUiManager() {
 }

void AccountsTabUiManager::onAccountsListed(const QList<TailAccountInfo>& foundAccounts) {
    accounts = foundAccounts;
    ui->lstAccounts->clear();
    for (const auto& account : foundAccounts) {
        auto* pCurrent = new QListWidgetItem(account.tailnet + "\n" + account.account);
        pCurrent->setData(Qt::UserRole, account.id);
        ui->lstAccounts->addItem(pCurrent);
        pCurrent->setTextAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
    }
}

void AccountsTabUiManager::onTailStatusChanged(TailStatus* pTailStatus) {
    if (pTailStatus->user->id <= 0) {
        // Not logged in

        // Hide account details view, nothing to show
        ui->accountDetailsContainer->setVisible(false);
        return;
    }

    // Show account details view
    ui->accountDetailsContainer->setVisible(true);
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
