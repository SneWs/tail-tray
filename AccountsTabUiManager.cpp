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
    , pTailStatus(nullptr)
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

    connect(ui->chkRunAsExitNode, &QCheckBox::clicked, this, [this]() {
        ui->chkExitNodeAllowNetworkAccess->setEnabled(ui->chkRunAsExitNode->isChecked());
    });

    connect(ui->lstAccounts, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        // TODO: Get info about selected account
        auto accountId = item->data(Qt::UserRole).toString();
        TailAccountInfo account{};
        for (const auto& acc : accounts) {
            if (acc.id == accountId) {
                account = acc;
                break;
            }
    }

        if (account.account.endsWith('*')) {
            // This is the active account...
            onTailStatusChanged(pTailStatus);
        }
        else {
            // Secondary account not currently active...
            ui->accountDetailsContainer->setVisible(true);
            ui->lblUsername->setText(account.account);
            ui->lblTailnetName->setText(account.tailnet);
            ui->lblEmail->setText(account.account);
            ui->lblStatus->setText("Not running");
            ui->lblKeyExpiry->setText("");
        }
    });
}

    void AccountsTabUiManager::onAccountsListed(const QList<TailAccountInfo>& foundAccounts) {
    accounts = foundAccounts;
    ui->lstAccounts->clear();
    for (const auto& account : foundAccounts) {
        auto loginName = account.account;
        if (loginName.endsWith('*'))
            loginName = loginName.chopped(1);
        auto* pCurrent = new QListWidgetItem(account.tailnet + "\n" + loginName);
        pCurrent->setData(Qt::UserRole, account.id);
        ui->lstAccounts->addItem(pCurrent);
        pCurrent->setTextAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
    }
}

void AccountsTabUiManager::onTailStatusChanged(TailStatus* status) {
    pTailStatus = status;
    if (pTailStatus == nullptr || pTailStatus->user == nullptr || pTailStatus->user->id <= 0) {
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
