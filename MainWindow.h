#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>
#include <QTimer>

#include "TailRunner.h"
#include "TailSettings.h"
#include "TrayMenuManager.h"
#include "AccountsTabUiManager.h"
#include "./ui_MainWindow.h"

class AccountsTabUiManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void showSettingsTab();
    void showAccountsTab();
    void showAboutTab();

    void syncSettingsToUi() const;
    void syncSettingsFromUi();

    void userLoggedOut() { changeToState(TailState::NotLoggedIn); }

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<AccountsTabUiManager> accountsTabUi;
    std::unique_ptr<TrayMenuManager> pTrayManager;
    std::unique_ptr<TailRunner> pCurrentExecution;
    std::unique_ptr<TailStatus> pTailStatus;

    TailState eCurrentState;
    TailSettings settings;
    QList<TailAccountInfo> accounts;

private slots:
    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void settingsClosed();
    void loginFlowCompleted() const;

private:
    // Switch to the new state and return the prev (old) state back to caller
    TailState changeToState(TailState newState);
    void onTailStatusChanged(TailStatus* pNewStatus);
};

#endif // MAINWINDOW_H
