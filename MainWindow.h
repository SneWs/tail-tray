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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void showSettingsTab();
    void showAccountsTab();
    void showAboutTab();

private:
    Ui::MainWindow* ui;
    TrayMenuManager* pTrayManager;

    TailState eCurrentState;
    TailRunner* pCurrentExecution;
    TailStatus* pTailStatus;
    QTimer* pStatusCheckTimer;

    TailSettings settings;

private slots:
    void settingsClosed();

private:
    // Switch to the new state and return the prev (old) state back to caller
    TailState changeToState(TailState newState);
    void onTailStatusChanged(TailStatus* pNewStatus);

    void syncSettingsToUi();
    void syncSettingsFromUi();
};
#endif // MAINWINDOW_H
