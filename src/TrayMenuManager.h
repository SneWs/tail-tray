#ifndef TRAYMENUMANAGER_H
#define TRAYMENUMANAGER_H

#include <memory>

#include <QSystemTrayIcon>
#include <QMenu>
#include <QString>

#include "TailRunner.h"
#include "TailSettings.h"
#include "SysCommand.h"
#include "ScriptManager.h"

class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(TailSettings& s, TailRunner* runner, QObject* parent = nullptr);

    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void stateChangedTo(TailState newState, const TailStatus& pTailStatus);

    [[nodiscard]] QSystemTrayIcon* trayIcon() const { return pSysTray.get(); }

signals:
    void ipAddressCopiedToClipboard(const QString& ipAddress, const QString& hostname);

private:
    QList<TailAccountInfo> accounts;
    TailSettings& settings;
    TailRunner* pTailRunner;
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
    ScriptManager scriptManager;

private:
    void buildNotLoggedInMenu() const;
    void buildNotConnectedMenu(const TailStatus& pTailStatus) const;
    void buildConnectedMenu(const TailStatus& pTailStatus);
    void buildAccountsMenu() const;

    void setupWellKnownActions() const;
};

#endif //TRAYMENUMANAGER_H
