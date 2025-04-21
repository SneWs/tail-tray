//
// Created by marcus on 2024-07-11.
//

#include "AccountsTabUiManager.h"

#include <QDesktopServices>
#include <QUrl>

#include "./ui_MainWindow.h"
#include "MainWindow.h"
#include "models/Models.h"

AccountsTabUiManager::AccountsTabUiManager(Ui::MainWindow* u, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , ui(u)
    , pTailRunner(runner)
    , pTailStatus()
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
        bool isActiveAccount = false;
        if (loginName.endsWith('*')) {
            isActiveAccount = true;
            loginName = loginName.chopped(1);
        }

        auto* pCurrent = new QListWidgetItem(account.tailnet + "\n" + loginName);
        pCurrent->setData(Qt::UserRole, account.id);
        if (isActiveAccount && foundAccounts.count() > 1) {
            QFont font = pCurrent->font();
            font.setBold(true);
            pCurrent->setFont(font);
        }
        ui->lstAccounts->addItem(pCurrent);
        pCurrent->setTextAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
    }
}

void AccountsTabUiManager::onTailStatusChanged(const TailStatus& status) {
    pTailStatus = status;
    showAccountDetails(false);

    if (pTailStatus.user.id <= 0) {
        // Not logged in
        return;
    }

    // Show account details view
    ui->lblUsername->setText(pTailStatus.user.displayName);
    ui->lblTailnetName->setText(pTailStatus.user.loginName);
    ui->lblEmail->setText(pTailStatus.user.loginName);
    ui->lblStatus->setText(pTailStatus.backendState);
    ui->lblKeyExpiry->setText("");

    // Show the key expiry date in a more human-readable format
    const auto now = QDateTime::currentDateTime();
    const auto daysToExpiry = now.daysTo(pTailStatus.self.keyExpiry);
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

    showAccountDetails(true);

    if (!pTailStatus.user.profilePicUrl.isEmpty()) {
        // ui->lblUsername->setPixmap(QPixmap(pTailStatus->user->profilePicUrl));
    }
}

void AccountsTabUiManager::showAccountDetails(bool show) {
    if (!show)
        ui->lblUsername->setText(tr("Select an account in the list to view details"));
    ui->lblUsername->setVisible(true);
    
    ui->lblTailnetName->setVisible(show);
    ui->lblTailnetNameTitle->setVisible(show);
    ui->lblEmail->setVisible(show);
    ui->lblEmailTitle->setVisible(show);
    ui->lblStatus->setVisible(show);
    ui->lblStatusTitle->setVisible(show);
    ui->lblKeyExpiry->setVisible(show);
    ui->lblKeyExpiryTitle->setVisible(show);
    ui->lblUserImage->setVisible(show);
    ui->btnLogout->setVisible(show);
    ui->btnAdminConsole->setVisible(show);
    ui->btnReAuthenticate->setVisible(show);
}
