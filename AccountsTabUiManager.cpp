//
// Created by marcus on 2024-07-11.
//

#include "AccountsTabUiManager.h"

#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>

#include "./ui_MainWindow.h"
#include "models.h"

AccountsTabUiManager::AccountsTabUiManager(Ui::MainWindow* u, QObject* parent)
    : QObject(parent)
    , ui(u)
{

    connect(ui->btnAdminConsole, &QPushButton::clicked,
        this, [this]() {
            QDesktopServices::openUrl(QUrl("https://login.tailscale.com/admin"));
        }
    );
}

AccountsTabUiManager::~AccountsTabUiManager() {
 }

void AccountsTabUiManager::onTailStatusChanged(TailStatus* pTailStatus) {
    if (pTailStatus->user->id <= 0)
        return; // Not logged in

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
    ui->lblKeyExpiry->setText(pTailStatus->self->keyExpiry.toString(Qt::DateFormat::ISODate));
    if (!pTailStatus->user->profilePicUrl.isEmpty()) {
        // ui->lblUsername->setPixmap(QPixmap(pTailStatus->user->profilePicUrl));
    }
}
