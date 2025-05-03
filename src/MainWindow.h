#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QNetworkInformation>
#include <QMap>
#include <QDateTime>

#include "./ui_MainWindow.h"
#include "TailRunner.h"
#include "TailSettings.h"
#include "TrayMenuManager.h"
#include "AccountsTabUiManager.h"
#include "TailFileReceiver.h"
#include "NetworkStateMonitor.h"
#include "IpnWatcher.h"
#include "models/TailStatus.h"
#include "PleaseWaitDlg.h"
#include "NotificationsManager.h"

#if defined(DAVFS_ENABLED)
#include "TailDriveUiManager.h"
#endif

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
    std::unique_ptr<PleaseWaitDlg> pLoginInProgressDlg;
    TailStatus pTailStatus{};
    TailDnsStatus pDnsStatus{};
    std::unique_ptr<TailFileReceiver> pFileReceiver;
    std::unique_ptr<NetworkStateMonitor> pNetworkStateMonitor;
    std::unique_ptr<IpnWatcher> pIpnWatcher;
#if defined(DAVFS_ENABLED)
    std::unique_ptr<TailDriveUiManager> pTailDriveUiManager;
#endif
    std::unique_ptr<NotificationsManager> pNotificationsManager;

    TailState eCurrentState;
    TailSettings settings;
    QList<TailAccountInfo> accounts;

    QMap<QString, QDateTime> seenWarningsAndErrors;

private slots:
    void tailscaleIsInstalled(bool installed);
    void settingsReadyToRead();
    void dnsStatusUpdated(const TailDnsStatus& dnsStatus);
    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void onCommandError(const QString& error, bool isSudoRequired);
    void settingsClosed();
    void loginFlowStarting(const QString& loginUrl);
    void loginFlowCompleted(bool success = true);
    void onIpnEvent(const IpnEventData& eventData);
    void ipAddressCopiedToClipboard(const QString& ipAddress, const QString& hostname);

#if defined(DAVFS_ENABLED)
    void drivesListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg);
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

    // Warnings, Errors etc reporting (Toast messages)
    void showWarningMessage(const QString& title, const QString& message, bool timeLimited = true);
    void showErrorMessage(const QString& title, const QString& message, bool timeLimited = true);

private:
    // Switch to the new state and return the prev (old) state back to caller
    TailState changeToState(TailState newState);
    void onTailStatusChanged(const TailStatus& pNewStatus);

    static bool shallowCheckForNetworkAvailable();

    [[nodiscard]] static bool isTailDriveFileAlreadySetup();

protected:
    void showEvent(QShowEvent* event) override;
};

#endif // MAINWINDOW_H
