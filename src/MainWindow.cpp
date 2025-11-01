#include "MainWindow.h"

#include <QDir>
#include <QFile>

#include "Paths.h"

#include <QList>
#include <QFileDialog>
#include <QMessageBox>
#include <QShowEvent>
#include <memory>
#include <QDesktopServices>

#include "KnownValues.h"
#include "AdvertiseRoutesDlg.h"
#include "DnsSettingsDlg.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , accountsTabUi(nullptr)
    , pTrayManager(nullptr)
    , pCurrentExecution(nullptr)
    , pLoginInProgressDlg(nullptr)
    , pFileReceiver(nullptr)
    , pNetworkStateMonitor(std::make_unique<NetworkStateMonitor>(this))
    , pIpnWatcher(std::make_unique<IpnWatcher>(this))
    , eCurrentState(TailState::NoAccount)
    , settings(this)
    , isFixingOperator(false)
{
    ui->setupUi(this);

    pCurrentExecution = std::make_unique<TailRunner>(settings, this);
    accountsTabUi = std::make_unique<AccountsTabUiManager>(ui.get(), pCurrentExecution.get(), this);
    pScriptManager = std::make_unique<ScriptManager>(settings, this);
    pTrayManager = std::make_unique<TrayMenuManager>(settings, pCurrentExecution.get(), pScriptManager.get(), this);
    pNotificationsManager = std::make_unique<NotificationsManager>(pTrayManager.get(), this);

    // Remove the tail drive tab by default
    ui->tabWidget->removeTab(2);

// Davfs or not
#if defined(DAVFS_ENABLED)
    pTailDriveUiManager = std::make_unique<TailDriveUiManager>(ui.get(), pCurrentExecution.get(), this);
    
    // Make sure to adjust tail drive based on the check state
    if (settings.tailDriveEnabled()) {
        ui->tabWidget->insertTab(3, ui->tabTailDrive, QIcon::fromTheme("drive-removable-media"), "Tail Drive");
    }

    connect(pCurrentExecution.get(), &TailRunner::driveListed, this, &MainWindow::drivesListed);
#else
    settings.tailDriveEnabled(false);
    ui->chkUseTailDrive->setChecked(false);
    ui->chkUseTailDrive->setVisible(false);
    ui->lblTailDrive->setVisible(false);
    ui->tailDriveLayoutRow->parentWidget()->layout()->removeItem(ui->tailDriveLayoutRow);
#endif

    connect(ui->btnSelectTailFileDefaultSaveLocation, &QPushButton::clicked,
        this, &MainWindow::onShowTailFileSaveLocationPicker);

    connect(ui->btnSelectTailScripFileSaveLocation, &QPushButton::clicked,
        this, &MainWindow::onShowTailScriptFileSaveLocationPicker);

    connect(pCurrentExecution.get(), &TailRunner::tailscaleIsInstalled, this, &MainWindow::tailscaleIsInstalled);
    connect(pCurrentExecution.get(), &TailRunner::settingsRead, this, &MainWindow::settingsReadyToRead);
    connect(pCurrentExecution.get(), &TailRunner::dnsStatusRead, this, &MainWindow::dnsStatusUpdated);
    connect(pCurrentExecution.get(), &TailRunner::accountsListed, this, &MainWindow::onAccountsListed);
    connect(pCurrentExecution.get(), &TailRunner::commandError, this, &MainWindow::onCommandError);
    connect(pCurrentExecution.get(), &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);
    connect(pCurrentExecution.get(), &TailRunner::loginFlowStarting, this, &MainWindow::loginFlowStarting);
    connect(pCurrentExecution.get(), &TailRunner::loginFlowCompleted, this, &MainWindow::loginFlowCompleted);
    connect(pCurrentExecution.get(), &TailRunner::fileSent, this, &MainWindow::fileSentToDevice);

    connect(pNetworkStateMonitor.get(), &NetworkStateMonitor::netCheckCompleted, this, &MainWindow::netCheckCompleted);
    connect(pIpnWatcher.get(), &IpnWatcher::eventReceived, this, &MainWindow::onIpnEvent);

    connect(ui->btnAdvertiseRoutes, &QPushButton::clicked, this, &MainWindow::showAdvertiseRoutesDialog);
    connect(ui->btnTailscaleDnsSettings, &QPushButton::clicked, this, &MainWindow::showDnsSettingsDialog);

    connect(pTrayManager.get(), &TrayMenuManager::ipAddressCopiedToClipboard, this, &MainWindow::ipAddressCopiedToClipboard);

    changeToState(TailState::NotLoggedIn);

    // NOTE! We first check and validate that the tailscale binary / service / daemon is installed
    // if we get a positive outcome from that, we will bootstrap the rest of the flow.
    pCurrentExecution->checkIfInstalled();

    connect(ui->btnSettingsClose, &QPushButton::clicked, this, &MainWindow::settingsClosed);

    // Make sure the settings tab is selected by default
    ui->tabWidget->setCurrentIndex(1);

    // Disable Tailnet lock UI for now
    ui->lblTailnetLockStatus->setEnabled(false);
    ui->lblTailnetLockTitle->setEnabled(false);
    ui->btnManageTailnetLocks->setEnabled(false);

    // Script manager hooks
    connect(pScriptManager.get(), &ScriptManager::availableScriptsChanged,
        this, &MainWindow::onScriptManagerScriptsChanged);

#if defined(WINDOWS_BUILD)
    // On windows this looks like crap, so don't use it
    ui->twNetworkStatus->setAlternatingRowColors(false);
#endif

    pScriptManager->tryInstallWatcher();
}

void MainWindow::shutdown() {
    if (pIpnWatcher != nullptr)
        pIpnWatcher->stop();

    if (pNetworkStateMonitor != nullptr)
        pNetworkStateMonitor->shutdown();

    if (pFileReceiver != nullptr)
        pFileReceiver->shutdown();

    if (pCurrentExecution != nullptr)
        pCurrentExecution->shutdown();
}

void MainWindow::showSettingsTab() {
    ui->tabWidget->setCurrentIndex(1);
    showNormal();
}

void MainWindow::showAccountsTab() {
    ui->tabWidget->setCurrentIndex(0);
    showNormal();
}

void MainWindow::showAboutTab() {
    auto tabIndex = 3;
    if (settings.tailDriveEnabled())
        tabIndex = 4;

    ui->tabWidget->setCurrentIndex(tabIndex);
    showNormal();
}

void MainWindow::showNetworkStatusTab() {
    ui->tabWidget->setCurrentIndex(2);
    showNormal();
}

void MainWindow::tailscaleIsInstalled(bool installed) {
    if (!installed) {
        qDebug() << "WARNING! Tailscale isn't installed. We will not be able to continue running Tail Tray!";
        qDebug() << "You can download and install tailscale from https://tailscale.com";

        changeToState(TailState::NotLoggedIn);
        showNormal();

        QMessageBox::warning(this, tr("Error"), tr("It does not look like you have installed Tailscale on this machine.\nOr tailscale isn't in your PATH.\n\nYou can download and install tailscale from https://tailscale.com"));

        close();
        // Since we are getting the signal from within a process, delay the exit a bit to allow the invoking process to 
        // complete before exiting the application
        QTimer::singleShot(500, [this]() {
            QApplication::exit(-1);
        });
        return;
    }

    qDebug() << "Tailscale seems to be installed, let's bootstrap and run!";

    // NOTE: The bootstrap to get this started is as follows:
    // 1. Read settings from Tailscale daemon
    // 2. Once that is successfully read, it will internally call getAccounts()
    // 3. Once getAccounts() have returned it will once again internally call getStatus()
    // 4. Once getStatus() returns we are in a running state, eg logged in and connected OR logged out OR logged in and disconnected etc...
    pCurrentExecution->bootstrap();
    pIpnWatcher->start();
}

void MainWindow::settingsReadyToRead() {
    auto tailscalePrefs = pCurrentExecution->currentSettings();

    // qDebug() << "Settings recv from Tailscale:";
    // qDebug() << "Operator: " << tailscalePrefs->operatorUser;
    // qDebug() << "Hostname: " << tailscalePrefs->hostname;
    // qDebug() << "Logged out: " << tailscalePrefs->loggedOut;
    // qDebug() << "Net filer kind: " << tailscalePrefs->netfilterKind;
    // qDebug() << "Net filer mode: " << tailscalePrefs->netfilterMode;
    // qDebug() << "Posture checking: " << tailscalePrefs->postureChecking;
    // qDebug() << "Route all: " << tailscalePrefs->routeAll;
    // qDebug() << "Shields up: " << tailscalePrefs->shieldsUp;
    // qDebug() << "Want running: " << tailscalePrefs->wantRunning;
    // qDebug() << "Allow single hosts: " << tailscalePrefs->allowSingleHosts;
    // qDebug() << "No stateful filtering: " << tailscalePrefs->noStatefulFiltering;
    // qDebug() << "Run web client: " << tailscalePrefs->runWebClient;
    // qDebug() << "Control panel URL: " << tailscalePrefs->controlURL;
    // qDebug() << "Corporate DNS: " << tailscalePrefs->corpDNS;
    // qDebug() << "Internal exit node prior: " << tailscalePrefs->internalExitNodePrior;
    // qDebug() << "Run SSH: " << tailscalePrefs->runSSH;
    // qDebug() << "No SNAT: " << tailscalePrefs->noSNAT;
    // qDebug() << "Allow Exit node LAN Access: " << tailscalePrefs->exitNodeAllowLANAccess;
    auto isExitNode = tailscalePrefs.isExitNode();
    // if (isExitNode) {
    //     qDebug() << "Advertise routes (We are exit node and advertising)";
    //     for (const auto& r : tailscalePrefs->advertiseRoutes)
    //         qDebug() << "\tRoute " << r;
    // }

    //qDebug() << "Exit node Id: " << tailscalePrefs->exitNodeId;
    // if (!tailscalePrefs->exitNodeId.isEmpty()) {
    //     if (pTailStatus != nullptr && pTailStatus->peers.count() > 0) {
    //         for (const auto& p : pTailStatus->peers) {
    //             if (p->id == tailscalePrefs->exitNodeId) {
    //                 qDebug() << "\tExit node by name: " << p->getShortDnsName();
    //             }
    //         }
    //     }
    // }
    // qDebug() << "Exit node Ip: " << tailscalePrefs->exitNodeIp;

#if defined(WINDOWS_BUILD)
    //qDebug() << "Notepad URLs: " << tailscalePrefs->notepadURLs;
#endif

    // Sync settings with local settings
    settings.allowIncomingConnections(!tailscalePrefs.shieldsUp);
    settings.useTailscaleDns(tailscalePrefs.corpDNS);
    settings.exitNodeAllowLanAccess(tailscalePrefs.exitNodeAllowLANAccess);
    settings.advertiseAsExitNode(isExitNode);
    settings.autoUpdateTailscale(tailscalePrefs.autoUpdate_Apply);

    syncSettingsToUi();

    if (isFixingOperator)
        return;

    // Make sure current user is operator
    auto isUserOperator = tailscalePrefs.operatorUser == qEnvironmentVariable("USER");
    if (!isUserOperator && !tailscalePrefs.loggedOut) {
        isFixingOperator = true;

        const auto response = QMessageBox::warning(nullptr,
           "Failed to run command",
           "To be able to control tailscale you need to be root or set yourself as operator. Do you want to set yourself as operator?",
           QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

        if (response == QMessageBox::Ok) {
            pCurrentExecution->setOperator();
        }

        isFixingOperator = false;
    }
    else {
        isFixingOperator = false;

        // Operator confirmed, lets continue the flow
        pCurrentExecution->getAccounts();
    }
}

void MainWindow::dnsStatusUpdated(const TailDnsStatus& dnsStatus) {
    pDnsStatus = dnsStatus;

    // qDebug() << "DNS Status recv from Tailscale:";
    //
    // qDebug() << "Split DNS routes:";
    // for (const auto& r : dnsStatus->splitDnsRoutes)
    //     qDebug() << "\t" << r.first << ": " << r.second;
    //
    // qDebug() << "Search Domains";
    // for (const auto& d : dnsStatus->searchDomains)
    //     qDebug() << "\t" << d;
}

void MainWindow::onAccountsListed(const QList<TailAccountInfo>& foundAccounts) {
    accounts = foundAccounts;
    pTrayManager->onAccountsListed(foundAccounts);
    pTrayManager->stateChangedTo(eCurrentState, pTailStatus);

    accountsTabUi->onAccountsListed(foundAccounts);
    accountsTabUi->onTailStatusChanged(pTailStatus);
}

void MainWindow::onCommandError(const QString& error, bool isSudoRequired) {
    if (isSudoRequired)
    {
        if (isFixingOperator)
            return;

        isFixingOperator = true;

        const auto& prefs = pCurrentExecution->currentSettings();
        if (prefs.loggedOut) {
            return;
        }

        const auto response = QMessageBox::warning(this, tr("Sudo required"),
            error + tr("\n\nTo use Tail Tray you need to be set as operator. Do you want to set yourself as operator now?"),
            QMessageBox::Ok | QMessageBox::Cancel);

        if (response == QMessageBox::Ok)
            pCurrentExecution->setOperator();

        return;
    }

    QMessageBox::warning(this, tr("Error running tailscale"), error);
}

void MainWindow::settingsClosed() {
    syncSettingsFromUi();

    if (eCurrentState == TailState::Connected)
        pCurrentExecution->start();

    hide();

    pScriptManager->tryInstallWatcher();
}

void MainWindow::loginFlowStarting(const QString& loginUrl) {
    qDebug() << "Login flow starting... Will send user to" << (loginUrl.isEmpty() ? "Waiting for URL..." : loginUrl);

    if (!loginUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(loginUrl));
    }

    // Show main window - accounts tab
    showAccountsTab();

    // And create and show dialog for login flow...
    if (pLoginInProgressDlg == nullptr) {
        pLoginInProgressDlg = std::make_unique<PleaseWaitDlg>(tr("Please wait, login flow is running..."));
        pLoginInProgressDlg->setModal(true);
        pLoginInProgressDlg->setFixedSize(300, 200);
        connect(pLoginInProgressDlg.get(), &PleaseWaitDlg::userCancelled, this, [this]() {
            qDebug() << "User cancelling Login flow...";

            pCurrentExecution->cancelLoginFlow();
            loginFlowCompleted(false);
        });
    }

    pLoginInProgressDlg->show();
}

void MainWindow::loginFlowCompleted(bool success) {
    qDebug() << "Login flow completed with" << (success ? "Success" : "Failure/Cancelled");

    pLoginInProgressDlg.reset();

    if (success) {
        // And bring the settings tab front and center
        showSettingsTab();

        pCurrentExecution->start();
    }
    else {
        // Failure
        QMessageBox::warning(this, tr("Login failure"), tr("Login failed. Please try again"),
            QMessageBox::Ok, QMessageBox::Ok);
    }
}

void MainWindow::onIpnEvent(const IpnEventData& eventData) {
    if (eventData.Health.Warnings.networkStatus.ImpactsConnectivity) {
        if (eventData.Health.Warnings.networkStatus.WarnableCode == "network-status") {
            if (eCurrentState != TailState::Connected) {
                if (!eventData.Health.Warnings.networkStatus.Text.isEmpty()) {
                    showWarningMessage(eventData.Health.Warnings.networkStatus.Title, eventData.Health.Warnings.networkStatus.Text);
                }
            }
        }
    }
    else {
        if (eventData.Health.Warnings.networkStatus.Severity == "warning") {
            if (!eventData.Health.Warnings.networkStatus.Text.isEmpty())
                showWarningMessage(eventData.Health.Warnings.networkStatus.Title, eventData.Health.Warnings.networkStatus.Text);
        }
        else if (eventData.Health.Warnings.networkStatus.Severity == "error") {
            if (!eventData.Health.Warnings.networkStatus.Text.isEmpty())
                showErrorMessage(eventData.Health.Warnings.networkStatus.Title, eventData.Health.Warnings.networkStatus.Text);
        }
    }

    pCurrentExecution->bootstrap();
}

void MainWindow::ipAddressCopiedToClipboard(const QString& ipAddress, const QString& hostname) {
        pNotificationsManager->showNotification(tr("IP address copied"),
            "IP Address " + ipAddress + " for " + hostname + " have been copied to clipboard!");
}

#if defined(DAVFS_ENABLED)
void MainWindow::drivesListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg) {
    if (error) {
        pTailStatus.drivesConfigured = false;
        qDebug() << errorMsg;
        qDebug() << "To read more about configuring tail drives, see https://tailscale.com/kb/1369/taildrive";

        QMessageBox::information(nullptr,
            tr("Tail Drive - Error"),
            tr("Tail drives needs to be enabled in ACL. Please go to the admin dashboard.\n\n") + errorMsg,
            QMessageBox::Ok);

        return; // Nothing more to do here
    }

    pTailStatus.drivesConfigured = true;

    // Store available drives
    pTailStatus.drives = QList(drives);

    // Refresh the tray icon menus
    pTrayManager->stateChangedTo(eCurrentState, pTailStatus);

    // And the UI for it
    pTailDriveUiManager->stateChangedTo(eCurrentState, pTailStatus);
}
#endif

void MainWindow::fileSentToDevice(bool success, const QString& errorMsg, void* userData) const {
    if (!success) {
        pNotificationsManager->showErrorNotification(tr("Failed to send file"), errorMsg);
    }

    if (userData == nullptr) {
        return;
    }

    auto userDataStr = static_cast<QString*>(userData);
    QFileInfo fileInfo(*userDataStr);
    pNotificationsManager->showFileNotification(tr("File sent"),
        tr("The file %1 has been sent!").arg(*userDataStr), fileInfo);

    delete userDataStr;
}

void MainWindow::startListeningForIncomingFiles() {
    if (pFileReceiver != nullptr)
        pFileReceiver.reset();

    pFileReceiver = std::make_unique<TailFileReceiver>(settings.tailFilesDefaultSavePath(), this);
    connect(pFileReceiver.get(), &TailFileReceiver::fileReceived,
        this, &MainWindow::onTailnetFileReceived);

    connect(pFileReceiver.get(), &TailFileReceiver::errorListening,
        this, [this](const QString& errorMsg) {
            pNotificationsManager->showErrorNotification(tr("Error"), errorMsg);
        });
}

void MainWindow::onTailnetFileReceived(QString filePath) const {
    const QFileInfo file(filePath);
    pNotificationsManager->showFileNotification(tr("File received"),
        tr("File %1 has been saved in %2").arg(file.fileName()).arg(file.absolutePath()),
            file);
}

void MainWindow::onShowTailFileSaveLocationPicker() {
    QFileDialog dlg(this, tr("Select folder"), ui->txtTailFilesDefaultSavePath->text());
    dlg.setFileMode(QFileDialog::FileMode::Directory);
    dlg.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
    dlg.setOption(QFileDialog::Option::ShowDirsOnly, true);

    auto result = dlg.exec();
    if (result == QFileDialog::Accepted) {
        const auto& selection = dlg.selectedFiles();
        ui->txtTailFilesDefaultSavePath->setText(selection.first().trimmed());
        settings.tailFilesDefaultSavePath(selection.first().trimmed());
    }

    startListeningForIncomingFiles();
}

void MainWindow::onShowTailScriptFileSaveLocationPicker() {
    QFileDialog dlg(this, tr("Select folder"), ui->txtTailScriptFilesSavePath->text());
    dlg.setFileMode(QFileDialog::FileMode::Directory);
    dlg.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
    dlg.setOption(QFileDialog::Option::ShowDirsOnly, true);

    auto result = dlg.exec();
    if (result == QFileDialog::Accepted) {
        const auto& selection = dlg.selectedFiles();
        ui->txtTailScriptFilesSavePath->setText(selection.first().trimmed());
        settings.tailScriptFilesSavePath(selection.first().trimmed());
    }

    startListeningForIncomingFiles();
}

namespace
{
    static QList<QTableWidgetItem*> netCheckWidgetItems{};

    static void cleanupDisposableNetCheckWidgetItems() {
        for (auto* wi : netCheckWidgetItems) {
            delete wi;
        }
        netCheckWidgetItems.clear();
    }
}

void MainWindow::netCheckCompleted(bool success, const QMap<QString, QString>& results, QList<QPair<QString, float>>& latencies) const {
    cleanupDisposableNetCheckWidgetItems();

    ui->twNetworkStatus->clear();
    ui->twNetworkStatus->setColumnCount(2);
    ui->twNetworkStatus->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    ui->twNetworkStatus->setRowCount(static_cast<int>(results.count() + latencies.count() + 1));

    // For latencies, we want to sort on lowest latencies first
    // Sort the list based on the second element (the value)
    std::sort(latencies.begin(), latencies.end(), [](const QPair<QString, float>& a, const QPair<QString, float>& b) {
        return a.second < b.second;
    });

    int i = 0;
    for (auto it = results.begin(); it != results.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        auto* w1 = new QTableWidgetItem(key);
        auto* w2 = new QTableWidgetItem(value);
        netCheckWidgetItems.push_back(w1);
        netCheckWidgetItems.push_back(w2);

        ui->twNetworkStatus->setItem(i, 0, w1);
        ui->twNetworkStatus->setItem(i, 1, w2);

        if (key == "Nearest DERP") {
            // Find the DERP Latency
            for (const auto& derp : latencies) {
                if (derp.first == value) {
                    ui->twNetworkStatus->item(i, 1)->setText(value + " (" + QString::number(derp.second) + "ms)");
                    break;
                }
            }
        }

        ++i;
    }

    // Add DERP header
    auto* w1 = new QTableWidgetItem("DERP Latencies");
    auto* w2 = new QTableWidgetItem("");
    netCheckWidgetItems.push_back(w1);
    netCheckWidgetItems.push_back(w2);

    ui->twNetworkStatus->setItem(i, 0, w1);
    ui->twNetworkStatus->setItem(i, 1, w2);
    ++i;

    for (auto it = latencies.begin(); it != latencies.end(); ++it) {
        const auto& key = it->first;
        const auto& value = it->second;
        w1 = new QTableWidgetItem(key);
        netCheckWidgetItems.push_back(w1);
        ui->twNetworkStatus->setItem(i, 0, w1);

        QString val;
        if (value >= 999999)
            val = "-";
        else
            val = QString::number(value) + "ms";

        w2 = new QTableWidgetItem(val);
        netCheckWidgetItems.push_back(w2);
        ui->twNetworkStatus->setItem(i, 1, w2);
        ++i;
    }
}

void MainWindow::showAdvertiseRoutesDialog() const {
    auto knownRoutes = pCurrentExecution->currentSettings().getFilteredAdvertiseRoutes();
    
    AdvertiseRoutesDlg dlg(knownRoutes);
    dlg.setWindowIcon(windowIcon());
    auto result = dlg.exec();
    if (result != QDialog::Accepted)
        return;
    
    const auto& routes = dlg.getDefinedRoutes();
    pCurrentExecution->advertiseRoutes(routes);
}

void MainWindow::showDnsSettingsDialog() const {    
    DnsSettingsDlg dlg(pDnsStatus, ui->chkUseTailscaleDns->isChecked());
    dlg.setWindowIcon(windowIcon());
    dlg.exec();

    ui->chkUseTailscaleDns->setChecked(dlg.isTailscaleDnsEnabled());
}

void MainWindow::showWarningMessage(const QString& title, const QString& message, bool timeLimited) {
    auto key = message.toLower().trimmed();
    if (timeLimited) {
        auto now = QDateTime::currentDateTime();
        if (seenWarningsAndErrors.contains(key)) {
            auto lastShown = seenWarningsAndErrors[message];
            auto diff = lastShown.secsTo(now);
            if (diff < 60 * 60) // 1 hour
                return; // No point in showing the same message again
        }
        else {
            seenWarningsAndErrors.insert(key, now);
        }
    }

    pNotificationsManager->showWarningNotification(title, message);
}

void MainWindow::showErrorMessage(const QString& title, const QString& message, bool timeLimited) {
    auto key = message.toLower().trimmed();
    if (timeLimited) {
        auto now = QDateTime::currentDateTime();
        if (seenWarningsAndErrors.contains(key)) {
            auto lastShown = seenWarningsAndErrors[message];
            auto diff = lastShown.secsTo(now);
            if (diff < 60 * 60) // 1 hour
                return; // No point in showing the same message again
        }
        else {
            seenWarningsAndErrors.insert(key, now);
        }
    }

    pNotificationsManager->showErrorNotification(title, message);
}

void MainWindow::onScriptManagerScriptsChanged() const {
    // Trigger re-load of menus
    pTrayManager->stateChangedTo(eCurrentState, pTailStatus);
}

bool MainWindow::isTailDriveFileAlreadySetup() {
    auto homeDavFsSecret = KnownValues::getTailDriveFilePath();

    // Create the .davfs2 folder if it doesn't exist
    QFile davFsSecret(homeDavFsSecret);
    if (!davFsSecret.open(QIODevice::ReadWrite)) {
        davFsSecret.close();
        return false;
    }

    auto fileContent = QString(davFsSecret.readAll());
    auto lines = fileContent.split('\n', Qt::SkipEmptyParts);
    for (const auto& line : lines) {
        if (line.trimmed().startsWith('#'))
            continue; // Comment
        if (line.contains(KnownValues::tailDavFsUrl, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    // Read settings, and it will be synced to UI once read
    pCurrentExecution->readSettings();
}

TailState MainWindow::changeToState(TailState newState)
{
    auto retVal = eCurrentState;
    auto didChangeState = eCurrentState != newState;
    eCurrentState = newState;

    if (eCurrentState == TailState::NotLoggedIn)
    {
        // Clear the status
        pTailStatus = TailStatus{};
        pTailStatus.user = TailUser{};
    }

    if (didChangeState) {
        // If we're not connected, don't allow showing/changing tabs etc
        if (newState == TailState::Connected) {
            ui->tabWidget->setCurrentIndex(1);
            ui->tabNetworkStatus->setDisabled(false);
            ui->tabSettings->setDisabled(false);
            ui->tabTailDrive->setDisabled(false);

            setWindowIcon(QIcon(":/icons/tray-on.png"));
            if (didChangeState) {
                seenWarningsAndErrors.clear();
            }
        }
        else {
            ui->tabNetworkStatus->setDisabled(true);
            ui->tabSettings->setDisabled(true);
            ui->tabTailDrive->setDisabled(true);
            ui->tabWidget->setCurrentIndex(0);

            setWindowIcon(QIcon(":/icons/tray-off.png"));
        }
    }

    pTrayManager->stateChangedTo(newState, pTailStatus);
    accountsTabUi->onTailStatusChanged(pTailStatus);

    if (eCurrentState == TailState::NotConnected) {
        // Nothing more to do
        return retVal;
    }

    auto isOnline = eCurrentState == TailState::Connected;
    if (isOnline && retVal != TailState::Connected) {
        startListeningForIncomingFiles();

#if defined(DAVFS_ENABLED)
    pTailDriveUiManager->stateChangedTo(newState, pTailStatus);
    if (settings.tailDriveEnabled()) {
        pCurrentExecution->listDrives();
    }
#endif
    }

    return retVal;
}

void MainWindow::onTailStatusChanged(const TailStatus& pNewStatus)
{
    // NOTE: Make sure to capture any stored drive data from prev drive listing
    //       if not we will lose track of the drives
    QList<TailDriveInfo> drives = pTailStatus.drives;
    pTailStatus = pNewStatus;
    pTailStatus.drives = drives;

    const auto& tailscalePrefs = pCurrentExecution->currentSettings();

    if (pTailStatus.user.id > 0) {
        if (pTailStatus.self.online)
            changeToState(TailState::Connected);
        else
            changeToState(TailState::NotConnected);

        if (pTailStatus.health.count() > 0)
        {
            auto now = QDateTime::currentDateTime();
            QString str{};
            for (const auto& s : pTailStatus.health)
                str += s + "\n";

            if (str.length() > 1)
                showWarningMessage(tr("Warning"), str);
        }

        auto formattedVersion = pTailStatus.version.mid(0, pTailStatus.version.indexOf("-"));
        ui->lblVersionNumber->setText(tr("Version ") + formattedVersion);
    }
    else {
        changeToState(TailState::NotLoggedIn);
    }

    accountsTabUi->onTailStatusChanged(pTailStatus);
    pTailDriveUiManager->stateChangedTo(eCurrentState, pTailStatus);
}

bool MainWindow::shallowCheckForNetworkAvailable() {
    auto* inst = QNetworkInformation::instance();
    if (inst->reachability() == QNetworkInformation::Reachability::Online)
        return true;

    return false;
}

void MainWindow::syncSettingsToUi() const {
    ui->chkAllowIncomingCnx->setChecked(settings.allowIncomingConnections());
    ui->chkUseTailscaleDns->setChecked(settings.useTailscaleDns());
    ui->chkRunAsExitNode->setChecked(settings.advertiseAsExitNode());
    ui->chkAcceptRoutes->setChecked(settings.acceptRoutes());
    ui->chkExitNodeAllowNetworkAccess->setChecked(settings.exitNodeAllowLanAccess());
    ui->chkStartOnLogin->setChecked(settings.startOnLogin());
    ui->chkUseTailDrive->setChecked(settings.tailDriveEnabled());
    ui->txtTailDriveDefaultMountPath->setText(settings.tailDriveMountPath());
    ui->txtTailFilesDefaultSavePath->setText(settings.tailFilesDefaultSavePath());
    ui->txtTailScriptFilesSavePath->setText(settings.tailScriptFilesSavePath());

#if defined(WINDOWS_BUILD)
    // Windows does support auto update
    ui->chkAutoUpdateTailscale->setChecked(settings.autoUpdateTailscale());
#else
    // Auto update is only supported on select platforms, Linux is not one of them (in general)
    // So we hide the option on Linux
    ui->lblTailScaleAutoUpdate->setVisible(false);
    ui->chkAutoUpdateTailscale->setVisible(false);
    ui->lblTailScaleAutoUpdate->setEnabled(false);
    ui->chkAutoUpdateTailscale->setEnabled(false);
    ui->tabSettings->layout()->removeItem(ui->layoutAutoUpdate);
#endif

    auto advertisedRoutes = pCurrentExecution->currentSettings().getFilteredAdvertiseRoutes();
    if (advertisedRoutes.count() > 0) {
        ui->lblAdvertisingNumRoutes->setText(tr("Advertising %1 routes")
            .arg(advertisedRoutes.count())
        );
    }
    else {
        ui->lblAdvertisingNumRoutes->setText(tr("No routes advertised"));
    }

    // Do we have a startup entry?
    auto configDir = QDir::home();
    if (!configDir.cd(".config"))
        return; // That's odd, no .config folder...

    if (!configDir.exists("autostart")) {
        if (!configDir.mkdir("autostart"))
            return;
    }

    configDir.cd("autostart");
    ui->chkStartOnLogin->setChecked(QFile::exists(configDir.absolutePath() + "/tail-tray.desktop"));
}

void MainWindow::syncSettingsFromUi() {
    settings.allowIncomingConnections(ui->chkAllowIncomingCnx->isChecked());
    settings.useTailscaleDns(ui->chkUseTailscaleDns->isChecked());
    settings.advertiseAsExitNode(ui->chkRunAsExitNode->isChecked());
    settings.exitNodeAllowLanAccess(ui->chkExitNodeAllowNetworkAccess->isChecked());
    settings.startOnLogin(ui->chkStartOnLogin->isChecked());
    settings.tailDriveEnabled(ui->chkUseTailDrive->isChecked());
    settings.tailDriveMountPath(ui->txtTailDriveDefaultMountPath->text().trimmed());
    settings.acceptRoutes(ui->chkAcceptRoutes->isChecked());

#if defined(WINDOWS_BUILD)
    settings.autoUpdateTailscale(ui->chkAutoUpdateTailscale->isChecked());
#endif

    const QDir dir(ui->txtTailFilesDefaultSavePath->text().trimmed());
    if (dir.exists()) {
        settings.tailFilesDefaultSavePath(ui->txtTailFilesDefaultSavePath->text().trimmed());
        startListeningForIncomingFiles();
    }

    const QString scriptPath = ui->txtTailScriptFilesSavePath->text().trimmed();
    if (scriptPath.isEmpty()) {
        settings.tailScriptFilesSavePath("");
    } else {
        const QDir dir(scriptPath);
        if (dir.exists()) {
            settings.tailScriptFilesSavePath(scriptPath);
        } else {
                settings.tailScriptFilesSavePath("");
                ui->txtTailScriptFilesSavePath->clear();
            }
    }

    auto homeDir = QDir::home();
    auto targetFile = homeDir.absolutePath() + "/.config/autostart/tail-tray.desktop";
    if (settings.startOnLogin()) {
        if (!QFile::exists(targetFile)) {
            (void)homeDir.mkpath(".config/autostart");
            QFile::copy(QString(DATAROOTDIR)
                + QString("/applications/tail-tray.desktop"), targetFile);
        }
    }
    else {
        QFile::remove(targetFile);
    }

    settings.save();
}
