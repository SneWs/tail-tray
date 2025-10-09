#include "TrayMenuManager.h"
#include "ScriptManager.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDir>

#include "MainWindow.h"
#include "ManageDriveWindow.h"
#include "KnownValues.h"

#include <functional>
#include <algorithm>

namespace {
    static QList<QAction*> disposableConnectedMenuActions = {};
    static QList<QMenu*> disposableMenus = {};

    void cleanupDisposableActions() {
        for (QAction* ac : disposableConnectedMenuActions) {
            delete ac;
        }

        disposableConnectedMenuActions.clear();
    }

    void cleanupDisposableMenus() {
        for (QMenu* m : disposableMenus) {
            if (m)
                m->deleteLater();
        }

        disposableMenus.clear();
    }
}

TrayMenuManager::TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , settings(s)
    , pTailRunner(runner)
    , pSysCommand(std::make_unique<SysCommand>())
    , scriptManager(s)
{
    pTrayMenu = std::make_unique<QMenu>("Tail Tray");

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
    stateChangedTo(TailState::NotLoggedIn, TailStatus{});
}

void TrayMenuManager::onAccountsListed(const QList<TailAccountInfo>& foundAccounts) {
    accounts = foundAccounts;
}

void TrayMenuManager::stateChangedTo(TailState newState, const TailStatus& pTailStatus) {
    cleanupDisposableActions();
    cleanupDisposableMenus();

    switch (newState) {
        case TailState::Connected:
        case TailState::LoggedIn: {
            buildConnectedMenu(pTailStatus);
            break;
        }
        case TailState::NoAccount:
        case TailState::NotLoggedIn: {
            buildNotLoggedInMenu();
            break;
        }
        case TailState::NotConnected: {
            buildNotConnectedMenu(pTailStatus);
            break;
        }
        default:
            assert(!"Unhandled TailState status!");
    }
}

void TrayMenuManager::buildNotLoggedInMenu() const {
    pTrayMenu->clear();
    pTrayMenu->addAction(pLoginAction.get());
    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );
    pTrayMenu->addAction(pPreferences.get());
    pTrayMenu->addAction(pAbout.get());
    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );
    pTrayMenu->addAction(pQuitAction.get());

    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));
}

void TrayMenuManager::buildNotConnectedMenu(const TailStatus& pTailStatus) const {
    pTrayMenu->clear();
    pTrayMenu->addAction(pConnect.get());
    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );

    if (pTailStatus.user.id > 0)
        pThisDevice->setText(pTailStatus.user.loginName);
    pTrayMenu->addAction(pThisDevice.get());
    auto* actions = pTrayMenu->addMenu(tr("Custom Actions"));
    disposableMenus.push_back(actions);

    actions->addAction(pRestartTailscale.get());
    actions->addAction(pRefreshLocalDns.get());
    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );
    pTrayMenu->addAction(pPreferences.get());
    pTrayMenu->addAction(pAbout.get());
    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );
    pTrayMenu->addAction(pQuitAction.get());

    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));

    buildAccountsMenu();
}

void TrayMenuManager::buildConnectedMenu(const TailStatus& pTailStatus) {
    pTrayMenu->clear();
    pTrayMenu->addAction(pConnected.get());
    pTrayMenu->addAction(pDisconnect.get());

    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );

    pThisDevice->setText(pTailStatus.user.loginName);
    pTrayMenu->addAction(pThisDevice.get());

    auto* netDevs = pTrayMenu->addMenu(tr("Network devices"));
    disposableMenus.push_back(netDevs);
    for (const auto& dev : pTailStatus.peers) {

        // We do not want to show mullvad nodes in the network devices list at all.
        // TODO: Do we want a setting for showing them in a separate menu or something?
        if (dev.isMullvadExitNode()) {
            continue;
        }

        if (dev.id != pTailStatus.self.id) {
            auto name = dev.getShortDnsName();
            QAction* action;
            if (!dev.online) {
                action = netDevs->addAction(name + tr(" (offline)"));
                action->setEnabled(false);
            }
            else {
                const auto& ipAddresses = dev.tailscaleIPs;
                QString ipStr;

                if (ipAddresses.count() > 0) {
                    ipStr = " (" + ipAddresses.first() + ")";
                }

                auto* deviceMenu = netDevs->addMenu(name + ipStr);
                disposableMenus.push_back(deviceMenu);
                action = deviceMenu->addAction(tr("Copy IP address"));
                connect(action, &QAction::triggered, this, [this, dev, name](bool) {
                    QClipboard* clipboard = QApplication::clipboard();
                    const auto& str = dev.tailscaleIPs.first();
                    qDebug() << str;
                    clipboard->setText(str, QClipboard::Clipboard);

                    emit ipAddressCopiedToClipboard(str, name);
                });

                disposableConnectedMenuActions.push_back(
                    deviceMenu->addSeparator()
                );

                auto* sendFileAction = deviceMenu->addAction(tr("Send file"));
                disposableConnectedMenuActions.push_back(sendFileAction);
                connect(sendFileAction, &QAction::triggered, this, [this, name](bool) {
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
                        new QString(file));
                });

                storedDeviceIps.insert(dev.id, dev.tailscaleIPs.first());
                storedDeviceDns.insert(dev.id, dev.dnsName);
                rebuildScriptsMenu(dev.id, dev.tailscaleIPs.first(), dev.dnsName);
            }
            disposableConnectedMenuActions.push_back(action);
        }
    }

    // Tail drives
    if (settings.tailDriveEnabled()) {
        auto* drives = pTrayMenu->addMenu(tr("Drives"));
        disposableMenus.push_back(drives);
        if (!pTailStatus.drivesConfigured && pTailStatus.drives.count() < 1) {
            auto* action = drives->addAction(tr("Not configured, click to configure"));
            disposableConnectedMenuActions.push_back(action);
            connect(action, &QAction::triggered, this, [this](bool) {
                // Open help link in browser
                QDesktopServices::openUrl(QUrl("https://tailscale.com/kb/1369/taildrive"));
            });
        }
        else {
#if defined(DAVFS_ENABLED)
            auto* addAction = drives->addAction(tr("Add drive"));
            disposableConnectedMenuActions.push_back(addAction);
            connect(addAction, &QAction::triggered, this, [this](bool) {
                ManageDriveWindow wnd(TailDriveInfo{}, reinterpret_cast<QWidget*>(this->parent()));
                auto result = wnd.exec();
                if (result == QDialog::Accepted) {
                    pTailRunner->addDrive(wnd.driveInfo());
                }
            });

            auto* mountAction = drives->addAction(tr("Mount remote drives"));
            disposableConnectedMenuActions.push_back(mountAction);
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

            if (pTailStatus.drives.count() > 0) {
                disposableConnectedMenuActions.push_back(
                    drives->addSeparator()
                );
            }

            for (const auto& drive : pTailStatus.drives) {
                auto* driveMenu = drives->addMenu(drive.name);
                disposableMenus.push_back(driveMenu);

                auto* renameAction = driveMenu->addAction(tr("Rename"));
                disposableConnectedMenuActions.push_back(renameAction);
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
                disposableConnectedMenuActions.push_back(removeAction);
                connect(removeAction, &QAction::triggered, this, [this, drive](bool) {
                    pTailRunner->removeDrive(drive);
                });

                disposableConnectedMenuActions.push_back(
                    driveMenu->addSeparator()
                );

                auto* openAction = driveMenu->addAction(tr("Open in file manager"));
                disposableConnectedMenuActions.push_back(openAction);
                connect(openAction, &QAction::triggered, this, [this, drive](bool) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(drive.path)));
                });
            }
#endif
        }
    }

    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );

    auto* actions = pTrayMenu->addMenu(tr("Custom Actions"));
    disposableMenus.push_back(actions);
    actions->addAction(pRestartTailscale.get());
    actions->addAction(pRefreshLocalDns.get());

    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );

    auto* exitNodes = pTrayMenu->addMenu(tr("Exit nodes"));
    disposableMenus.push_back(exitNodes);

    exitNodes->addAction(pExitNodeNone.get());

    bool hasMullvadNodes = false;
    for (int i = 0; i < pTailStatus.peers.size(); i++) {
        const auto& dev = pTailStatus.peers[i];
        if (dev.id == pTailStatus.self.id) {
            // Ignore self
            continue;
        }

        if (dev.exitNodeOption) {
            if (dev.isMullvadExitNode()) {
                hasMullvadNodes = true;
                continue;
            }
            
            auto name = dev.getShortDnsName();
            if (!dev.online) {
                name += tr(" (offline)");
            }
            
            auto* action = exitNodes->addAction(name);
            disposableConnectedMenuActions.push_back(action);

            action->setCheckable(true);
            action->setChecked(dev.exitNode);
            action->setData(name);

            if (dev.exitNode) {
                pExitNodeNone->setChecked(false);
                pExitNodeNone->setEnabled(true);
            }

            // You can't use a exit node if you are advertising as exit node
            action->setEnabled(dev.online && !settings.advertiseAsExitNode());

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

    if (hasMullvadNodes) {
        disposableConnectedMenuActions.push_back(
            exitNodes->addSeparator()
        );

        auto* mullvadExitNodes = exitNodes->addMenu(tr("Mullvad Exit Nodes"));
        disposableMenus.push_back(mullvadExitNodes);

        auto mapByCountryAndCity = pTailStatus.getMullvadExitNodesByCountry();

        for (const auto& country : mapByCountryAndCity.keys()) {
            const auto& countryMap = mapByCountryAndCity[country];
            auto* countryMenu = mullvadExitNodes->addMenu(country);
            disposableMenus.push_back(countryMenu);

            for (const auto& city : countryMap.keys()) {
                auto cityMenu = countryMenu->addMenu(city);
                disposableMenus.push_back(cityMenu);

                const auto& peers = countryMap[city];
                for (const auto& peer : peers) {
                    auto dnsName = peer.getShortDnsName();
                    if (dnsName.isEmpty()) {
                        continue; // Skip if no DNS name
                    }
                    if (!peer.online) {
                        dnsName += tr(" (offline)");
                    }
                    
                    auto* action = cityMenu->addAction(dnsName);
                    disposableConnectedMenuActions.push_back(action);

                    action->setCheckable(true);
                    action->setChecked(peer.exitNode);
                    action->setData(peer.dnsName);

                    if (peer.exitNode) {
                        pExitNodeNone->setChecked(false);
                        pExitNodeNone->setEnabled(true);
                    }

                    // Only enable if online
                    action->setEnabled(peer.online);

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
        }
    }

    disposableConnectedMenuActions.push_back(
        exitNodes->addSeparator()
    );

    QAction* runExitNode = exitNodes->addAction(tr("Run as exit node"));
    disposableConnectedMenuActions.push_back(runExitNode);

    runExitNode->setCheckable(true);
    runExitNode->setChecked(settings.advertiseAsExitNode());
    connect(runExitNode, &QAction::triggered, this, [this, runExitNode](bool) {
        settings.advertiseAsExitNode(runExitNode->isChecked());
        settings.save();
        pTailRunner->start();
    });

    QAction* exitNodeAllowNwAccess = exitNodes->addAction(tr("Allow local network access"));
    disposableConnectedMenuActions.push_back(exitNodeAllowNwAccess);

    exitNodeAllowNwAccess->setCheckable(true);
    exitNodeAllowNwAccess->setChecked(settings.exitNodeAllowLanAccess());
    exitNodeAllowNwAccess->setEnabled(settings.advertiseAsExitNode());
    connect(exitNodeAllowNwAccess, &QAction::triggered, this, [this, exitNodeAllowNwAccess](bool) {
        settings.exitNodeAllowLanAccess(exitNodeAllowNwAccess->isChecked());
        settings.save();
        pTailRunner->start();
    });

    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );
    pTrayMenu->addAction(pPreferences.get());
    pTrayMenu->addAction(pAbout.get());
    disposableConnectedMenuActions.push_back(
        pTrayMenu->addSeparator()
    );
    pTrayMenu->addAction(pQuitAction.get());

    pSysTray->setIcon(QIcon(":/icons/tray-on.png"));
    buildAccountsMenu();
}

void TrayMenuManager::rebuildScriptsMenu(const QString& deviceId, const QString& ip, const QString& dnsName) {
    auto scripts = scriptManager.getDefinedScripts();

    if (!scripts.isEmpty() && !deviceScriptMenus.contains(deviceId)) {
        for (auto* m : disposableMenus) {
            if (m->title().contains(ip, Qt::CaseInsensitive) ||
                m->title().contains(dnsName, Qt::CaseInsensitive)) {

                auto* scriptsMenu = m->addMenu(tr("Scriptable Actions"));
                deviceScriptMenus.insert(deviceId, scriptsMenu);
                disposableMenus.push_back(scriptsMenu);

                storedDeviceIps.insert(deviceId, ip);
                storedDeviceDns.insert(deviceId, dnsName);
                break;
            }
        }
    }

    if (!deviceScriptMenus.contains(deviceId))
        return;

    QMenu* scriptsMenu = deviceScriptMenus.value(deviceId);
    scriptsMenu->clear();

    if (scripts.isEmpty())
        return;

    for (const auto& script : scripts) {
        QFileInfo fileInfo(script);
        QAction* action = scriptsMenu->addAction(fileInfo.baseName());
        connect(action, &QAction::triggered, this, [fileInfo, ip, dnsName]() {
            qDebug() << "Running script:" << fileInfo.absoluteFilePath();
            QProcess::startDetached(fileInfo.absoluteFilePath(), { ip, dnsName });
        });
    }
}

void TrayMenuManager::onScriptsUpdated() {
    auto scripts = scriptManager.getDefinedScripts();
    if (scripts.isEmpty())
        return;

    for (auto it = deviceScriptMenus.begin(); it != deviceScriptMenus.end(); ++it) {
        const QString& deviceId = it.key();
        rebuildScriptsMenu(deviceId, storedDeviceIps[deviceId], storedDeviceDns[deviceId]);
    }
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
        disposableConnectedMenuActions.push_back(accountAction);
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
        }
    );

    connect(pRestartTailscale.get(), &QAction::triggered, this, [this](bool) {
        pSysCommand->restartTailscaleDaemon();
    });

    connect(pRefreshLocalDns.get(), &QAction::triggered, this, [this](bool) {
        pSysCommand->refreshDns();
    });
}
