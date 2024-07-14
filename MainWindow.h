#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "TailRunner.h"
#include "TailSettings.h"
#include "TrayMenuManager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class AccountsTabUiManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void showSettingsTab();
    void showAccountsTab();
    void showAboutTab();

    void syncSettingsToUi() const;
    void syncSettingsFromUi();

    void userLoggedOut() { changeToState(TailState::NotLoggedIn); }

private:
    Ui::MainWindow* ui;
    AccountsTabUiManager* accountsTabUi;
    TrayMenuManager* pTrayManager;
    TailRunner* pCurrentExecution;
    TailStatus* pTailStatus;

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
