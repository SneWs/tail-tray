//
// Created by marcus on 2024-07-06.
//

#include <QApplication>
#include <QClipboard>

#include "TrayMenuManager.h"
#include "MainWindow.h"

TrayMenuManager::TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , settings(s)
    , pTailRunner(runner)
    , pStatusCheckTimer(nullptr)
    , pSysTray(nullptr)
    , pTrayMenu(nullptr)
    , pConnect(nullptr)
    , pDisconnect(nullptr)
    , pQuitAction(nullptr)
    , pLoginAction(nullptr)
    , pConnected(nullptr)
    , pLogoutAction(nullptr)
    , pPreferences(nullptr)
    , pAbout(nullptr)
    , pThisDevice(nullptr)
{
    pTrayMenu = new QMenu("Tailscale");

    // Make sure to restart the status check timer when the menu is shown
    // since this will give us some time before it will try to re-fresh the menu etc
    // NOTE: aboutToHide() is not used since it will not be triggered when the menu is closed/dismissed when focus is lost
    connect(pTrayMenu, &QMenu::aboutToShow, this, [this]() {
        pStatusCheckTimer->start();
    });

    pSysTray = new QSystemTrayIcon(this);
    pSysTray->setContextMenu(pTrayMenu);
    pSysTray->setToolTip("Tailscale");
    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));
    pSysTray->setVisible(true);

    pQuitAction = new QAction("Quit");
    pLoginAction = new QAction("Login");
    pLogoutAction = new QAction("Logout");
    pPreferences = new QAction("Preferences");
    pAbout = new QAction("About...");
    pConnected = new QAction("Connected");
    pConnected->setEnabled(false);
    pConnect = new QAction("Connect");
    pDisconnect = new QAction("Disconnect");
    pThisDevice = new QAction("This device");

    setupWellKnownActions();

    // Periodic status check
    pStatusCheckTimer = new QTimer(this);
    connect(pStatusCheckTimer, &QTimer::timeout, this, [this]() {
        pTailRunner->checkStatus();
    });
    pStatusCheckTimer->setSingleShot(false);
    pStatusCheckTimer->start(1000 * 30); // 30sec interval

    stateChangedTo(TailState::NotLoggedIn, nullptr);
}

TrayMenuManager::~TrayMenuManager()
{
    pStatusCheckTimer->stop();
    delete pStatusCheckTimer;

    delete pTrayMenu;
    delete pSysTray;
    delete pConnect;
    delete pDisconnect;
    delete pQuitAction;
    delete pLoginAction;
    delete pConnected;
    delete pLogoutAction;
    delete pPreferences;
    delete pAbout;
    delete pThisDevice;
}

void TrayMenuManager::stateChangedTo(TailState newState, TailStatus const* pTailStatus)
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
        case TailState::ConnectedWithExitNode:
            buildConnectedExitNodeMenu(pTailStatus);
            break;
        default:
            assert(!"Unhandled TailState status!");
    }
}

void TrayMenuManager::buildNotLoggedInMenu()
{
    pTrayMenu->clear();
    pTrayMenu->addAction(pLoginAction);
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pPreferences);
    pTrayMenu->addAction(pAbout);
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pQuitAction);

    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));
}

void TrayMenuManager::buildNotConnectedMenu(TailStatus const* pTailStatus)
{
    pTrayMenu->clear();
    pTrayMenu->addAction(pConnect);
    pTrayMenu->addSeparator();
    pThisDevice->setText(pTailStatus->user->loginName);
    pTrayMenu->addAction(pThisDevice);
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pPreferences);
    pTrayMenu->addAction(pAbout);
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pQuitAction);

    pSysTray->setIcon(QIcon(":/icons/tray-off.png"));
}

void TrayMenuManager::buildConnectedMenu(TailStatus const* pTailStatus)
{
    pTrayMenu->clear();
    pTrayMenu->addAction(pConnected);
    pTrayMenu->addAction(pDisconnect);
    pTrayMenu->addSeparator();
    pThisDevice->setText(pTailStatus->user->loginName);
    pTrayMenu->addAction(pThisDevice);

    pTrayMenu->addSeparator();
    pTrayMenu->addAction("This device: " + pTailStatus->self->hostName);

    auto* netDevs = pTrayMenu->addMenu("Network devices");
    for (auto* dev : pTailStatus->peers) {
        if (dev->id != pTailStatus->self->id) {
            auto name = dev->dnsName.replace(pTailStatus->magicDnsSuffix, "");
            name.chop(2);
            QAction* action;
            if (!dev->online) {
                action = netDevs->addAction(name + " (offline)");
            }
            else {
                action = netDevs->addAction(name);
            }

            connect(action, &QAction::triggered, this, [this, dev](bool) {
                QClipboard* clipboard = QApplication::clipboard();
                QString str = dev->tailscaleIPs.first();
                qDebug() << str;
                clipboard->setText(str, QClipboard::Clipboard);
                if (clipboard->supportsSelection()) {
                    clipboard->setText(str, QClipboard::Selection);
                }
            });
        }
    }

    pTrayMenu->addSeparator();
    auto* exitNodes = pTrayMenu->addMenu("Exit nodes");
    for (int i = 0; i < pTailStatus->peers.count(); i++) {
        auto* dev = pTailStatus->peers[i];
        if (dev->online && dev->id != pTailStatus->self->id && dev->exitNodeOption) {
            auto name = dev->dnsName.replace(pTailStatus->magicDnsSuffix, "");
            name.chop(2);
            auto* action = exitNodes->addAction(name);
            action->setCheckable(true);
            action->setChecked(dev->exitNode);
            action->setData(name);

            connect(action, &QAction::triggered, this, [this, action](bool) {
                if (action->isChecked()) {
                    auto devName = action->data().toString();
                    settings.exitNodeInUse(devName);
                    }
                else {
                    settings.exitNodeInUse("");
                }

                settings.save();
                pTailRunner->start();
            });
        }
    }
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pPreferences);
    pTrayMenu->addAction(pAbout);
    pTrayMenu->addSeparator();
    pTrayMenu->addAction(pQuitAction);

    pSysTray->setIcon(QIcon(":/icons/tray-on.png"));
}

void TrayMenuManager::buildConnectedExitNodeMenu(TailStatus const* pTailStatus)
{

}

void TrayMenuManager::setupWellKnownActions() {
    connect(pLoginAction, &QAction::triggered, this, [this](bool) {
        pTailRunner->login();
    });

    connect(pConnect, &QAction::triggered, this, [this](bool) {
        pTailRunner->start();
    });

    connect(pDisconnect, &QAction::triggered, this, [this](bool) {
        pTailRunner->stop();
    });

    connect(pPreferences, &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showSettingsTab();
    });

    connect(pAbout, &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showAboutTab();
    });

    connect(pThisDevice, &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showAccountsTab();
    });

    connect(pQuitAction, &QAction::triggered, qApp, &QApplication::quit);

    connect(pSysTray, &QSystemTrayIcon::activated,
        this, [this](QSystemTrayIcon::ActivationReason reason) {
            auto* wnd = dynamic_cast<MainWindow*>(this->parent());
            if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
                if (wnd->isVisible())
                    wnd->hide();
                else {
                    wnd->syncSettingsToUi();
                    wnd->showSettingsTab();
                }
            }
            else if (reason == QSystemTrayIcon::ActivationReason::Context) {
                // Restart background status refresh
                pStatusCheckTimer->start();
            }
        }
    );
}
