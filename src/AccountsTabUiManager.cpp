//
// Created by marcus on 2024-07-11.
//

#include "AccountsTabUiManager.h"


#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QBuffer>

#include "./ui_MainWindow.h"
#include "MainWindow.h"
#include "models/Models.h"

AccountsTabUiManager::AccountsTabUiManager(Ui::MainWindow* u, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , ui(u)
    , pAddAccountButtonMenu(nullptr)
    , pTailRunner(runner)
    , pNetworkManager(std::make_unique<QNetworkAccessManager>(this))
    , profilePicCache()
{
    // Add account context menu/split button for control server
    pAddAccountButtonMenu = std::make_unique<QMenu>(ui->btnAddAccount);
    QAction* addAccount = pAddAccountButtonMenu->addAction(tr("Add account"));
    QAction* addWithCustomUrl = pAddAccountButtonMenu->addAction(tr("Custom Control Server URL"));
    ui->btnAddAccount->setMenu(pAddAccountButtonMenu.get());

    connect(ui->btnAdminConsole, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl("https://login.tailscale.com/admin"));
        }
    );

    connect(ui->btnLogout, &QPushButton::clicked, this, [this]() {
            pTailRunner->logout();
            if (ui->lstAccounts->selectedItems().count() > 0) {
                auto* selectedItem = ui->lstAccounts->selectedItems().first();
                int row = ui->lstAccounts->row(selectedItem);
                if (row >= 0) {
                    auto* item = ui->lstAccounts->takeItem(row);
                    delete item;
                }
            }

            showAccountDetails(false);
            auto* wnd = dynamic_cast<MainWindow*>(this->parent());
            wnd->userLoggedOut();
            //wnd->hide();
        }
    );

    connect(addAccount, &QAction::triggered, this, [this]() {
            pTailRunner->login();
        }
    );

    connect(addWithCustomUrl, &QAction::triggered, this, [this]() {
            auto* wnd = dynamic_cast<MainWindow*>(this->parent());
            bool ok = false;
            QString url = QInputDialog::getText(wnd, tr("Custom Control Server URL"),
                tr("URL:"), QLineEdit::Normal, "https://", &ok);

            if (ok && !url.isEmpty()) {
                pTailRunner->login(url);
            }
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

            ui->btnLogout->setVisible(true);
            ui->btnReAuthenticate->setVisible(true);
            ui->btnAdminConsole->setVisible(true);
            ui->btnSwitchToAccount->setVisible(false);
        }
        else {
            // Nth account not currently active...
            ui->lblUsername->setText(account.account);
            ui->lblTailnetName->setText(account.tailnet);
            ui->lblEmail->setText(account.account);
            ui->lblStatus->setText("Not running");
            ui->lblKeyExpiry->setText("");

            ui->btnAdminConsole->setVisible(false);
			ui->btnLogout->setVisible(false);
			ui->btnReAuthenticate->setVisible(false);
            ui->btnSwitchToAccount->setVisible(true);
        }
    });

    connect(ui->btnSwitchToAccount, &QPushButton::clicked, this, [this]() {
        auto* item = ui->lstAccounts->currentItem();
        if (!item) {
            return;
        }

        auto accountId = item->data(Qt::UserRole).toString();
        pTailRunner->switchAccount(accountId);
    });

    ui->btnSwitchToAccount->setVisible(false);
}

void AccountsTabUiManager::onAccountsListed(const QList<TailAccountInfo>& foundAccounts) {
    accounts = foundAccounts;
    ui->lstAccounts->clear();
	ui->btnSwitchToAccount->setVisible(false);
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
        
        pCurrent->setTextAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
        ui->lstAccounts->addItem(pCurrent);
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
}

void AccountsTabUiManager::showAccountDetails(bool show) {
    if (!show)
        ui->lblUsername->setText(tr("Select an account in the list to view details"));

    ui->lblUsername->setVisible(show);
    
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

    if (!show) {
        ui->lblUsername->setPixmap(QPixmap()); // Clear pixmap if no URL
        return;
    }

    // Load profile picture if URL is not empty
    if (!pTailStatus.user.profilePicUrl.isEmpty()) {
        if (profilePicCache.find(pTailStatus.user.profilePicUrl) != profilePicCache.end()) {
            // Use cached pixmap
            ui->lblUsername->setPixmap(profilePicCache[pTailStatus.user.profilePicUrl].scaled(ui->lblUsername->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }

        QUrl imageUrl(pTailStatus.user.profilePicUrl);
        QNetworkRequest request(imageUrl);
        QNetworkReply* reply = pNetworkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imageData = reply->readAll();
                QPixmap pixmap;
                if (pixmap.loadFromData(imageData)) {
                    profilePicCache[pTailStatus.user.profilePicUrl] = pixmap; // Cache the pixmap
                    ui->lblUsername->setPixmap(pixmap.scaled(ui->lblUsername->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
            reply->deleteLater();
        });
    }
    else {
        profilePicCache[pTailStatus.user.profilePicUrl] = QPixmap(); // Cache empty pixmap for empty URL
        ui->lblUsername->setPixmap(QPixmap()); // Clear pixmap if no URL
    }
}
