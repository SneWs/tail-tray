//
// Created by marcus on 2024-07-06.
//

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>

#include "TrayMenuManager.h"

#include <QDir>

#include "MainWindow.h"
#include "ManageDriveWindow.h"
#include "KnownValues.h"

TrayMenuManager::TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , settings(s)
    , pTailRunner(runner)
    , pSysCommand(std::make_unique<SysCommand>())
{
    pTrayMenu = std::make_unique<QMenu>("Tail Tray");

    // Make sure to restart the status check timer when the menu is shown
    // since this will give us some time before it will try to re-fresh the menu etc
    // NOTE: aboutToHide() is not used since it will not be triggered when the menu is closed/dismissed when focus is lost
    connect(pTrayMenu.get(), &QMenu::aboutToShow, this, [this]() {
        pStatusCheckTimer->start();
    });

    pSysTray = std::make_unique<QSystemTrayIcon>(this);
    pSysTray->setContextMenu(pTrayMenu.get());
    pSysTray->setToolTip("Tailscale");
    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));
    pSysTray->setVisible(true);

    pQuitAction = std::make_unique<QAction>(tr("Quit"));
    pLoginAction = std::make_unique<QAction>(tr("Login"));
    pLogoutAction = std::make_unique<QAction>(tr("Logout"));
    pPreferences = std::make_unique<QAction>(tr("Preferences"));
    pAbout = std::make_unique<QAction>(tr("About..."));
    pConnected = std::make_unique<QAction>(tr("Connected"));
    pConnected->setEnabled(false);
    pConnect = std::make_unique<QAction>(tr("Connect"));
    pDisconnect = std::make_unique<QAction>(tr("Disconnect"));
    pThisDevice = std::make_unique<QAction>(tr("This device"));
    pExitNodeNone = std::make_unique<QAction>(tr("None"));
    pExitNodeNone->setCheckable(true);
    pExitNodeNone->setChecked(true);
    pExitNodeNone->setEnabled(false);
    pRefreshLocalDns = std::make_unique<QAction>(tr("Refresh Local DNS"));
    pRestartTailscale = std::make_unique<QAction>(tr("Restart Tailscale"));

    setupWellKnownActions();

    // Periodic status check
    pStatusCheckTimer = std::make_unique<QTimer>(this);
    connect(pStatusCheckTimer.get(), &QTimer::timeout, this, [this]() {
        pTailRunner->checkStatus();
    });
    pStatusCheckTimer->setSingleShot(false);
    pStatusCheckTimer->start(1000 * 30); // 30sec interval

    stateChangedTo(TailState::NotLoggedIn, nullptr);
}

void TrayMenuManager::onAccountsListed(const QList<TailAccountInfo>& foundAccounts) {
    accounts = foundAccounts;
}

void TrayMenuManager::stateChangedTo(TailState newState, TailStatus const* pTailStatus) const
{
    switch (newState) {
        case TailState::Connected:
        case TailState::LoggedIn: {
            buildConnectedMenu(pTailStatus);
            pStatusCheckTimer->start();
            break;
        }
        case TailState::NoAccount:
        case TailState::NotLoggedIn: {
            buildNotLoggedInMenu();
            pStatusCheckTimer->stop();
            break;
        }
        case TailState::NotConnected: {
            buildNotConnectedMenu(pTailStatus);
            pStatusCheckTimer->stop();
            break;
        }
        default:
            assert(!"Unhandled TailState status!");
    }
}

void TrayMenuManager::buildNotLoggedInMenu() const {
    pTrayMenu->clear();
    pTrayMenu->addAction(pLoginAction.get());
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pPreferences.get());
    pTrayMenu->addAction(pAbout.get());
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pQuitAction.get());

    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));
}

void TrayMenuManager::buildNotConnectedMenu(TailStatus const* pTailStatus) const {
    pTrayMenu->clear();
    pTrayMenu->addAction(pConnect.get());
    pTrayMenu->addSeparator();
    if (pTailStatus != nullptr && pTailStatus->user != nullptr)
        pThisDevice->setText(pTailStatus->user->loginName);
    pTrayMenu->addAction(pThisDevice.get());
    auto* actions = pTrayMenu->addMenu(tr("Custom Actions"));
    actions->addAction(pRestartTailscale.get());
    actions->addAction(pRefreshLocalDns.get());
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pPreferences.get());
    pTrayMenu->addAction(pAbout.get());
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pQuitAction.get());

    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));

    buildAccountsMenu();
}

void TrayMenuManager::buildConnectedMenu(TailStatus const* pTailStatus) const {
    pTrayMenu->clear();
    pTrayMenu->addAction(pConnected.get());
    pTrayMenu->addAction(pDisconnect.get());

    pTrayMenu->addSeparator();
    pThisDevice->setText(pTailStatus->user->loginName);
    pTrayMenu->addAction(pThisDevice.get());

    auto* netDevs = pTrayMenu->addMenu(tr("Network devices"));
    for (const auto& dev : pTailStatus->peers) {
        if (dev->id != pTailStatus->self->id) {
            auto name = dev->getShortDnsName();
            QAction* action;
            if (!dev->online) {
                action = netDevs->addAction(name + tr(" (offline)"));
                action->setEnabled(false);
            }
            else {
                const auto& ipAddresses = dev->tailscaleIPs;
                QString ipStr;

                if (ipAddresses.count() > 0) {
                    ipStr = " (" + ipAddresses.first() + ")";
                }

                auto* deviceMenu = netDevs->addMenu(name + ipStr);
                action = deviceMenu->addAction(tr("Copy IP address"));
                connect(action, &QAction::triggered, this, [this, dev, name, ipStr](bool) {
                    QClipboard* clipboard = QApplication::clipboard();
                    const auto& str = dev->tailscaleIPs.first();
                    qDebug() << str;
                    clipboard->setText(str, QClipboard::Clipboard);
                    if (clipboard->supportsSelection()) {
                        clipboard->setText(str, QClipboard::Selection);
                    }

                    pSysTray->showMessage(tr("IP address copied"),
                        "IP Address " + ipStr + " for " + name + " have been copied to clipboard!",
                        QSystemTrayIcon::Information, 5000);
                });

                deviceMenu->addSeparator();
                auto* sendFileAction = deviceMenu->addAction(tr("Send file"));
                connect(sendFileAction, &QAction::triggered, this, [this, name](bool) {
                    // TODO: Open file dialog and send file via tailscale file cp ... target-device:
                    QFileDialog dialog(nullptr, "Send file to " + name, QDir::homePath(), "All files (*)");
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
                    pTailRunner->sendFile(name, file,
                        new QString("File " + file + " sent to " + name));
                });
            }
        }
    }

    // Tail drives
    if (settings.tailDriveEnabled()) {
        auto* drives = pTrayMenu->addMenu(tr("Drives"));
        if (!pTailStatus->drivesConfigured && pTailStatus->drives.count() < 1) {
            auto* action = drives->addAction(tr("Not configured, click to configure"));
            connect(action, &QAction::triggered, this, [this](bool) {
                // Open help link in browser
                QDesktopServices::openUrl(QUrl("https://tailscale.com/kb/1369/taildrive"));
            });
        }
        else {
#if defined(DAVFS_ENABLED)
            auto* addAction = drives->addAction(tr("Add drive"));
            connect(addAction, &QAction::triggered, this, [this](bool) {
                ManageDriveWindow wnd(TailDriveInfo{}, reinterpret_cast<QWidget*>(this->parent()));
                auto result = wnd.exec();
                if (result == QDialog::Accepted) {
                    pTailRunner->addDrive(wnd.driveInfo());
                }
            });

            auto* mountAction = drives->addAction(tr("Mount remote drives"));
            connect(mountAction, &QAction::triggered, this, [this](bool) {
                static QString remote(KnownValues::tailDavFsUrl);
                static QString fsType("davfs");
                const QString mountPath = settings.tailDriveMountPath();
                const QDir mountDir(mountPath);
                qDebug() << "Will try to mount " << fsType << " to local path " << mountPath;

                if (!mountDir.exists()) {
                    SysCommand mkDirCmd{};
                    mkDirCmd.makeDir(mountPath);
                    (void)mkDirCmd.waitForFinished();
                }

                SysCommand mountCmd{};
                mountCmd.mountFs(remote, mountPath, fsType, "", true);
                (void)mountCmd.waitForFinished();
            });

            if (pTailStatus->drives.count() > 0)
                drives->addSeparator();

            for (const auto& drive : pTailStatus->drives) {
                auto* driveMenu = drives->addMenu(drive.name);

                auto* renameAction = driveMenu->addAction(tr("Rename"));
                connect(renameAction, &QAction::triggered, this, [this, drive](bool) {
                    ManageDriveWindow wnd(drive, reinterpret_cast<QWidget*>(this->parent()));
                    auto result = wnd.exec();
                    if (result == QDialog::Accepted) {
                        const auto& newName = wnd.driveInfo().name;
                        if (newName.compare(drive.name, Qt::CaseInsensitive) != 0) {
                            pTailRunner->renameDrive(drive, newName);
                        }
                    }
                });

                auto* removeAction = driveMenu->addAction(tr("Stop Sharing"));
                connect(removeAction, &QAction::triggered, this, [this, drive](bool) {
                    pTailRunner->removeDrive(drive);
                });

                driveMenu->addSeparator();
                auto* openAction = driveMenu->addAction(tr("Open in file manager"));
                connect(openAction, &QAction::triggered, this, [this, drive](bool) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(drive.path)));
                });
            }
#endif
        }
    }

    pTrayMenu->addSeparator();

    auto* actions = pTrayMenu->addMenu(tr("Custom Actions"));
    actions->addAction(pRestartTailscale.get());
    actions->addAction(pRefreshLocalDns.get());

    pTrayMenu->addSeparator();
    auto* exitNodes = pTrayMenu->addMenu(tr("Exit nodes"));
    exitNodes->addAction(pExitNodeNone.get());
    for (int i = 0; i < pTailStatus->peers.count(); i++) {
        const auto& dev = pTailStatus->peers[i];
        if (dev->id != pTailStatus->self->id && dev->exitNodeOption) {
            auto name = dev->getShortDnsName();
            if (!dev->online)
                name += tr(" (offline)");
            auto* action = exitNodes->addAction(name);
            action->setCheckable(true);
            action->setChecked(dev->exitNode);
            action->setData(name);

            if (dev->exitNode) {
                pExitNodeNone->setChecked(false);
                pExitNodeNone->setEnabled(true);
            }

            // You can't use a exit node if you are advertising as exit node
            action->setEnabled(dev->online && !settings.advertiseAsExitNode());

            connect(action, &QAction::triggered, this, [this, action](bool) {
                auto devName = QString{};

                bool isChecked = action->isChecked();
                pExitNodeNone->setChecked(!isChecked);

                if (isChecked) {
                    devName = action->data().toString();
                }

                pExitNodeNone->setChecked(devName.isEmpty());
                pTailRunner->setExitNode(devName);
            });
        }
    }
    exitNodes->addSeparator();
    QAction* runExitNode = exitNodes->addAction(tr("Run as exit node"));
    runExitNode->setCheckable(true);
    runExitNode->setChecked(settings.advertiseAsExitNode());
    connect(runExitNode, &QAction::triggered, this, [this, runExitNode](bool) {
        settings.advertiseAsExitNode(runExitNode->isChecked());
        settings.save();
        pTailRunner->start();
    });

    QAction* exitNodeAllowNwAccess = exitNodes->addAction(tr("Allow local network access"));
    exitNodeAllowNwAccess->setCheckable(true);
    exitNodeAllowNwAccess->setChecked(settings.exitNodeAllowLanAccess());
    exitNodeAllowNwAccess->setEnabled(settings.advertiseAsExitNode());
    connect(exitNodeAllowNwAccess, &QAction::triggered, this, [this, exitNodeAllowNwAccess](bool) {
        settings.exitNodeAllowLanAccess(exitNodeAllowNwAccess->isChecked());
        settings.save();
        pTailRunner->start();
    });

    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pPreferences.get());
    pTrayMenu->addAction(pAbout.get());
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pQuitAction.get());

    pSysTray->setIcon(QIcon(":/icons/tray-on.png"));
    buildAccountsMenu();
}

void TrayMenuManager::buildAccountsMenu() const {
    if (pThisDevice->menu() == nullptr) {
        pThisDevice->setMenu(new QMenu());
    }

    pThisDevice->menu()->clear();
    for (const auto& acc : accounts) {
        const bool isActive = acc.account.endsWith('*');
        auto accountName = acc.account;
        if (isActive) {
            accountName = accountName.chopped(1);
            pThisDevice->setText(accountName);
        }

        auto accountAction = new QAction(acc.tailnet + " (" + accountName + ")");
        accountAction->setCheckable(true);
        accountAction->setChecked(isActive);

        pThisDevice->menu()->addAction(accountAction);
        accountAction->setData(acc.id);
        connect(accountAction, &QAction::triggered, this, [this, accountAction](bool) {
            pTailRunner->switchAccount(accountAction->data().toString());
        });
    }
}

void TrayMenuManager::setupWellKnownActions() const {
    connect(pLoginAction.get(), &QAction::triggered, this, [this](bool) {
        pTailRunner->login();
    });

    connect(pConnect.get(), &QAction::triggered, this, [this](bool) {
        pSysTray->showMessage(tr("Please wait"), tr("Connecting to your Tailscale network"),
            QSystemTrayIcon::MessageIcon::Information, 3000);

        pTailRunner->start();
    });

    connect(pDisconnect.get(), &QAction::triggered, this, [this](bool) {
        pTailRunner->stop();
    });

    connect(pPreferences.get(), &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showSettingsTab();
    });

    connect(pAbout.get(), &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showAboutTab();
    });

    connect(pThisDevice.get(), &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showAccountsTab();
    });

    connect(pExitNodeNone.get(), &QAction::triggered, this, [this](bool) {
        pExitNodeNone->setEnabled(false);
        pTailRunner->setExitNode("");
    });

    connect(pQuitAction.get(), &QAction::triggered, qApp, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->shutdown();
        qApp->quit();
    });

    connect(pSysTray.get(), &QSystemTrayIcon::activated,
        this, [this](QSystemTrayIcon::ActivationReason reason) {
            auto* wnd = dynamic_cast<MainWindow*>(this->parent());
            if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
                if (wnd->isVisible())
                    wnd->hide();
                else {
                    // NOTE: When settings are read, they will call settings to UI so will be in sync on show
                    pTailRunner->readSettings();
                    wnd->showSettingsTab();
                }
            }
            else if (reason == QSystemTrayIcon::ActivationReason::Context) {
                // Restart background status refresh
                pStatusCheckTimer->start();
            }
        }
    );

    connect(pRestartTailscale.get(), &QAction::triggered, this, [this](bool) {
        pSysCommand->restartTailscaleDaemon();
    });

    connect(pRefreshLocalDns.get(), &QAction::triggered, this, [this](bool) {
        pSysCommand->refreshDns();
    });
}
