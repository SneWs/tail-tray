//
// Created by marcus on 2024-07-06.
//

#ifndef TRAYMENUMANAGER_H
#define TRAYMENUMANAGER_H

#include <memory>

#include <QSystemTrayIcon>
#include <QMenu>
#include <QString>

#include "models.h"
#include "TailRunner.h"
#include "TailSettings.h"
#include "SysCommand.h"

class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent = nullptr);
    ~TrayMenuManager() final;

    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void stateChangedTo(TailState newState, TailStatus const* pTailStatus) const;

    [[nodiscard]] QSystemTrayIcon* trayIcon() const { return pSysTray; }

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
    QAction* pRefreshLocalDns;

    std::unique_ptr<SysCommand> pSysCommand;

private:
    void buildNotLoggedInMenu() const;
    void buildNotConnectedMenu(TailStatus const* pTailStatus) const;
    void buildConnectedMenu(TailStatus const* pTailStatus) const;
    void buildAccountsMenu() const;

    void setupWellKnownActions() const;
};



#endif //TRAYMENUMANAGER_H
