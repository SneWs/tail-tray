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

class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(TailRunner* runner, QObject* parent = nullptr);
    virtual ~TrayMenuManager();

    void stateChangedTo(TailState newState, TailStatus const* pTailStatus);

private:
    TailRunner* pTailRunner;
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

private:
    void buildNotLoggedInMenu();
    void buildNotConnectedMenu(TailStatus const* pTailStatus);
    void buildConnectedMenu(TailStatus const* pTailStatus);
    void buildConnectedExitNodeMenu(TailStatus const* pTailStatus);
};



#endif //TRAYMENUMANAGER_H
