#ifndef TRAYMENUMANAGER_H
#define TRAYMENUMANAGER_H

#include <memory>

#include <QSystemTrayIcon>
#include <QMenu>
#include <QString>

#include "models/Models.h"
#include "TailRunner.h"
#include "TailSettings.h"
#include "SysCommand.h"

class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent = nullptr);

    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void stateChangedTo(TailState newState, TailStatus const* pTailStatus) const;

    [[nodiscard]] QSystemTrayIcon* trayIcon() const { return pSysTray.get(); }

private:
    QList<TailAccountInfo> accounts;
    TailSettings& settings;
    TailRunner* pTailRunner;
    std::unique_ptr<QTimer> pStatusCheckTimer;
    std::unique_ptr<QSystemTrayIcon> pSysTray;
    std::unique_ptr<QMenu> pTrayMenu;
    std::unique_ptr<QAction> pQuitAction;
    std::unique_ptr<QAction> pLoginAction;
    std::unique_ptr<QAction> pLogoutAction;
    std::unique_ptr<QAction> pConnected;
    std::unique_ptr<QAction> pConnect;
    std::unique_ptr<QAction> pDisconnect;
    std::unique_ptr<QAction> pPreferences;
    std::unique_ptr<QAction> pAbout;
    std::unique_ptr<QAction> pThisDevice;
    std::unique_ptr<QAction> pExitNodeNone;
    std::unique_ptr<QAction> pRefreshLocalDns;
    std::unique_ptr<QAction> pRestartTailscale;
    std::unique_ptr<SysCommand> pSysCommand;

private:
    void buildNotLoggedInMenu() const;
    void buildNotConnectedMenu(TailStatus const* pTailStatus) const;
    void buildConnectedMenu(TailStatus const* pTailStatus) const;
    void buildAccountsMenu() const;

    void setupWellKnownActions() const;
};

#endif //TRAYMENUMANAGER_H
