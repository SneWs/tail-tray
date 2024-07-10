//
// Created by marcus on 2024-07-06.
//

#include <QApplication>
#include <QClipboard>

#include "TrayMenuManager.h"
#include "MainWindow.h"

TrayMenuManager::TrayMenuManager(TailRunner* runner, QObject* parent)
    : QObject(parent)
    , pTailRunner(runner)
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
{
    pTrayMenu = new QMenu("Tailscale");
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

    connect(pConnect, &QAction::triggered, this, [this](bool) {
        pTailRunner->start();
    });

    connect(pDisconnect, &QAction::triggered, this, [this](bool) {
        pTailRunner->stop();
    });

    connect(pPreferences, &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showSettingsTab();
        wnd->show();
    });

    connect(pAbout, &QAction::triggered, this, [this](bool) {
        auto* wnd = dynamic_cast<MainWindow*>(this->parent());
        wnd->showAboutTab();
        wnd->show();
    });

    connect(pQuitAction, &QAction::triggered, qApp, &QApplication::quit);

    stateChangedTo(TailState::NotLoggedIn, nullptr);
}

TrayMenuManager::~TrayMenuManager()
{
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
}

void TrayMenuManager::stateChangedTo(TailState newState, TailStatus const* pTailStatus)
{
    switch (newState) {
        case TailState::Connected:
        case TailState::LoggedIn:
             buildConnectedMenu(pTailStatus);
            break;
        case TailState::NoAccount:
        case TailState::NotLoggedIn:
            buildNotLoggedInMenu();
        break;
        case TailState::NotConnected:
            buildNotConnectedMenu(pTailStatus);
        break;
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
    pTrayMenu->addAction(pTailStatus->user->loginName);
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
    pTrayMenu->addAction(pTailStatus->user->loginName);
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
    for (auto* dev : pTailStatus->peers) {
        if (dev->online && dev->id != pTailStatus->self->id && dev->exitNodeOption) {
            auto name = dev->dnsName.replace(pTailStatus->magicDnsSuffix, "");
            name.chop(2);
            auto* action = exitNodes->addAction(name);
            action->setCheckable(true);
            action->setChecked(dev->exitNode);

            connect(action, &QAction::triggered, this, [this, dev, action](bool) {
                if (action->isChecked()) {
                    pTailRunner->useExitNode(dev);
                }
                else {
                    pTailRunner->useExitNode(nullptr);
                }
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