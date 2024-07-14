//
// Created by marcus on 2024-07-06.
//

#ifndef TRAYMENUMANAGER_H
#define TRAYMENUMANAGER_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QString>

#include "models.h"
#include "TailRunner.h"
#include "TailSettings.h"

class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent = nullptr);
    virtual ~TrayMenuManager();

    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void stateChangedTo(TailState newState, TailStatus const* pTailStatus);

    QSystemTrayIcon* trayIcon() const { return pSysTray; }

private:
    QList<TailAccountInfo> accounts;
    TailSettings& settings;
    TailRunner* pTailRunner;
    QTimer* pStatusCheckTimer;
    QSystemTrayIcon* pSysTray;
    QMenu* pTrayMenu;
    QAction* pQuitAction;
    QAction* pLoginAction;
    QAction* pLogoutAction;
    QAction* pConnected;
    QAction* pConnect;
    QAction* pDisconnect;
    QAction* pPreferences;
    QAction* pAbout;
    QAction* pThisDevice;
    QAction* pExitNodeNone;

private:
    void buildNotLoggedInMenu();
    void buildNotConnectedMenu(TailStatus const* pTailStatus);
    void buildConnectedMenu(TailStatus const* pTailStatus);
    void buildConnectedExitNodeMenu(TailStatus const* pTailStatus);

    void buildAccountsMenu();

    void setupWellKnownActions();
};



#endif //TRAYMENUMANAGER_H
