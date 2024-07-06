//
// Created by marcus on 2024-07-06.
//

#include <QApplication>

#include "TrayMenuManager.h"

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
    pTrayMenu->addAction("Network devices");
    pTrayMenu->addSeparator();
    pTrayMenu->addAction("Exit node (none)");
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