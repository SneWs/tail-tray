#include "DevicesTabUiManager.h"

#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMenu>
#include <QStringList>
#include <QTreeWidgetItem>

DevicesTabUiManager::DevicesTabUiManager(TailRunner* currentExecution, Ui::MainWindow* _ui, QObject* parent)
    : QObject(parent)
    , ui(_ui)
    , pCurrentExecution(currentExecution)
{
    // Context menu
    ui->tvDevices->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_ui->tvDevices, &QTreeWidget::customContextMenuRequested,
        this, &DevicesTabUiManager::onDevicesTreeContextMenuRequested);
}

void DevicesTabUiManager::syncToUi(const TailStatus& status)
{
    pTailStatus = status;

    ui->tvDevices->clear();
    ui->tvDevices->setHeaderLabels(QStringList() << "Device" << "Tailscale IPs");

    auto onlineItems = new QTreeWidgetItem(QStringList() << "Online");
    auto offlineItems = new QTreeWidgetItem(QStringList() << "Offline");

    ui->tvDevices->addTopLevelItem(onlineItems);
    ui->tvDevices->addTopLevelItem(offlineItems);
    ui->tvDevices->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tvDevices->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    auto currentUserId = status.user.id;
    auto index = 0;
    for (const auto& peer : status.peers)
    {
        // If the peer belongs to another user, show full DNS name
        auto displayName = peer.getShortDnsName();
        if (peer.userId != currentUserId) {
            displayName = peer.dnsName.endsWith('.') ? peer.dnsName.chopped(1) : peer.dnsName;
        }

        if (displayName.isEmpty()) {
            displayName = peer.hostName;

            if (displayName.isEmpty())
            {
                displayName = tr("<unknown>");
            }
        }

        auto* item = new QTreeWidgetItem(QStringList() << displayName << peer.tailscaleIPs[0]);
        item->setData(0, Qt::UserRole, index++);
        if (peer.online) {
            onlineItems->addChild(item);
        }
        else {
            offlineItems->addChild(item);
        }
    }

    onlineItems->setExpanded(true);
    offlineItems->setExpanded(true);
}

void DevicesTabUiManager::onDevicesTreeContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* selectedItem = ui->tvDevices->itemAt(pos);
    if (selectedItem == nullptr)
        return;

    const auto peerIndex = selectedItem->data(0, Qt::UserRole);
    if (!peerIndex.isValid() || peerIndex.toInt() < 0 ||
        peerIndex.toInt() >= pTailStatus.peers.size()) {
        return;
    }
    auto peer = pTailStatus.peers[peerIndex.toInt()];

    QMenu menu(ui->tvDevices);

    auto* sendFileAction = menu.addAction(tr("Send file"));
    auto name = peer.getShortDnsName();
    connect(sendFileAction, &QAction::triggered, this, [this, name](bool) {
        QFileDialog dialog(nullptr, "Send file to " + name, QDir::homePath(),
            "All files (*)");
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setViewMode(QFileDialog::Detail);
        auto result = dialog.exec();
        if (result != QDialog::Accepted)
            return;

        if (dialog.selectedFiles().count() < 1)
            return;

        const QString file = dialog.selectedFiles().first();
        qDebug() << "Will send file " << file << " to " << name;

        // The user data will be cleaned up when the signal is triggered back to us
        pCurrentExecution->sendFile(name, file, new QString(file));
    });

    menu.addSeparator();

    auto* copyMenu = menu.addMenu(tr("Copy"));
    const auto addCopyMenu = [this, copyMenu, name](const QString& value,
                                                    bool isIpAddress) {
        auto* valueMenu = copyMenu->addAction(value);
        connect(valueMenu, &QAction::triggered, this,
            [this, value, name, isIpAddress](bool) {
                QClipboard* clipboard = QGuiApplication::clipboard();
                clipboard->setText(value);
                if (isIpAddress) {
                    emit ipAddressCopiedToClipboard(value, name);
                }
            });
    };

    for (const auto& ip : peer.tailscaleIPs) {
        addCopyMenu(ip, true);
    }

    addCopyMenu(name, false);
    auto fullHostname = peer.dnsName;
    if (fullHostname.endsWith('.')) {
        fullHostname.chop(1);
    }
    if (fullHostname.isEmpty()) {
        fullHostname = peer.hostName;
    }
    if (fullHostname != name) {
        addCopyMenu(fullHostname, false);
    }

    menu.exec(ui->tvDevices->viewport()->mapToGlobal(pos));
}