#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>
#include <QTimer>
#include <QNetworkInformation>
#include <QMap>
#include <QDateTime>

#include "TailRunner.h"
#include "TailSettings.h"
#include "TrayMenuManager.h"
#include "AccountsTabUiManager.h"
#include "TailFileReceiver.h"
#include "./ui_MainWindow.h"
#include "NetworkStateMonitor.h"
#include "models/TailStatus.h"

class AccountsTabUiManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void shutdown();

    void showSettingsTab();
    void showAccountsTab();
    void showAboutTab();
    void showNetworkStatusTab();

    void syncSettingsToUi() const;
    void syncSettingsFromUi();

    void userLoggedOut() { changeToState(TailState::NotLoggedIn); }

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<AccountsTabUiManager> accountsTabUi;
    std::unique_ptr<TrayMenuManager> pTrayManager;
    std::unique_ptr<TailRunner> pCurrentExecution;
    std::unique_ptr<TailStatus> pTailStatus;
    std::unique_ptr<TailDnsStatus> pDnsStatus;
    std::unique_ptr<TailFileReceiver> pFileReceiver;
    std::unique_ptr<NetworkStateMonitor> pNetworkStateMonitor;

    TailState eCurrentState;
    TailSettings settings;
    QList<TailAccountInfo> accounts;

    QMap<QString, QDateTime> seenWarnings;

private slots:
    void settingsReadyToRead();
    void dnsStatusUpdated(TailDnsStatus* dnsStatus);
    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void onCommandError(const QString& error, bool isSudoRequired);
    void settingsClosed();
    void loginFlowCompleted() const;
    void onNetworkReachabilityChanged(QNetworkInformation::Reachability newReachability);

#if defined(DAVFS_ENABLED)
    void drivesListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg) const;

    // Tail drive
    void addTailDriveButtonClicked() const;
    void removeTailDriveButtonClicked() const;
    void selectTailDriveMountPath() const;
    void fixTailDriveDavFsSetup() const;
#endif

    // Send file
    void fileSentToDevice(bool success, const QString& errorMsg, void* userData) const;
    void startListeningForIncomingFiles();
    void onTailnetFileReceived(QString filePath) const;
    void onShowTailFileSaveLocationPicker();

    // Network reports
    void netCheckCompleted(bool success, const QMap<QString, QString>& results, QList<QPair<QString, float>>& latencies) const;

    // Advertise routes and other network settings
    void showAdvertiseRoutesDialog() const;
    void showDnsSettingsDialog() const;

private:
    // Switch to the new state and return the prev (old) state back to caller
    TailState changeToState(TailState newState);
    void onTailStatusChanged(TailStatus* pNewStatus);

    static bool shallowCheckForNetworkAvailable();
    void setupNetworkCallbacks() const;

    [[nodiscard]] static bool isTailDriveFileAlreadySetup();

#if defined(DAVFS_ENABLED)
    void tailDrivesToUi() const;
#endif

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // MAINWINDOW_H
