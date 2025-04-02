#include <QDir>
#include <QFile>

#include "MainWindow.h"
#include "Paths.h"

#include <QList>
#include <QFileDialog>
#include <QMessageBox>

#include "ManageDriveWindow.h"
#include "KnownValues.h"
#include "AdvertiseRoutesDlg.h"
#include "DnsSettingsDlg.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , accountsTabUi(nullptr)
    , pTrayManager(nullptr)
    , pCurrentExecution(nullptr)
    , pTailStatus(nullptr)
    , pFileReceiver(nullptr)
    , eCurrentState(TailState::NoAccount)
    , pNetworkStateMonitor(std::make_unique<NetworkStateMonitor>(this))
    , pIpnWatcher(std::make_unique<IpnWatcher>(this))
    , pDnsStatus(std::make_unique<TailDnsStatus>())
#if defined(DAVFS_ENABLED)
    , pTailDriveUiManager()
#endif
    , settings(this)
{
    ui->setupUi(this);

    pCurrentExecution = std::make_unique<TailRunner>(settings, this);

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

    connect(pCurrentExecution.get(), &TailRunner::settingsRead, this, &MainWindow::settingsReadyToRead);
    connect(pCurrentExecution.get(), &TailRunner::dnsStatusRead, this, &MainWindow::dnsStatusUpdated);
    connect(pCurrentExecution.get(), &TailRunner::accountsListed, this, &MainWindow::onAccountsListed);
    connect(pCurrentExecution.get(), &TailRunner::commandError, this, &MainWindow::onCommandError);
    connect(pCurrentExecution.get(), &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);
    connect(pCurrentExecution.get(), &TailRunner::loginFlowCompleted, this, &MainWindow::loginFlowCompleted);
    connect(pCurrentExecution.get(), &TailRunner::fileSent, this, &MainWindow::fileSentToDevice);

    connect(pNetworkStateMonitor.get(), &NetworkStateMonitor::netCheckCompleted, this, &MainWindow::netCheckCompleted);
    connect(pIpnWatcher.get(), &IpnWatcher::eventReceived, this, &MainWindow::onIpnEvent);

    connect(ui->btnAdvertiseRoutes, &QPushButton::clicked, this, &MainWindow::showAdvertiseRoutesDialog);
    connect(ui->btnTailscaleDnsSettings, &QPushButton::clicked, this, &MainWindow::showDnsSettingsDialog);

    accountsTabUi = std::make_unique<AccountsTabUiManager>(ui.get(), pCurrentExecution.get(), this);
    pTrayManager = std::make_unique<TrayMenuManager>(settings, pCurrentExecution.get(), this);

    changeToState(TailState::NotConnected);

    // NOTE: The bootstrap to get this started is as follows:
    // 1. Read settings from Tailscale daemon
    // 2. Once that is successfully read, it will internally call getAccounts()
    // 3. Once getAccounts() have returned it will once again internally call getStatus()
    // 4. Once getStatus() returns we are in a running state, eg logged in and connected OR logged out OR logged in and disconnected etc...
    pCurrentExecution->bootstrap();

    connect(ui->btnSettingsClose, &QPushButton::clicked, this, &MainWindow::settingsClosed);

    // Make sure the settings tab is selected by default
    ui->tabWidget->setCurrentIndex(1);

    // Disable Tailnet lock UI for now
    ui->lblTailnetLockStatus->setEnabled(false);
    ui->lblTailnetLockTitle->setEnabled(false);
    ui->btnManageTailnetLocks->setEnabled(false);

    pIpnWatcher->start();

#if defined(WINDOWS_BUILD)
    // On windows this looks like crap, so don't use it
    ui->twNetworkStatus->setAlternatingRowColors(false);
#endif
}

void MainWindow::shutdown() {
    pIpnWatcher->stop();

    pNetworkStateMonitor->shutdown();
    pFileReceiver->shutdown();
    pCurrentExecution->shutdown();
}

void MainWindow::showSettingsTab() {
    ui->tabWidget->setCurrentIndex(1);
    show();
}

void MainWindow::showAccountsTab() {
    ui->tabWidget->setCurrentIndex(0);
    show();
}

void MainWindow::showAboutTab() {
    auto tabIndex = 3;
    if (settings.tailDriveEnabled())
        tabIndex = 4;

    ui->tabWidget->setCurrentIndex(tabIndex);
    show();
}

void MainWindow::showNetworkStatusTab() {
    ui->tabWidget->setCurrentIndex(2);
    show();
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
    auto isExitNode = tailscalePrefs->isExitNode();
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
    settings.allowIncomingConnections(!tailscalePrefs->shieldsUp);
    settings.useTailscaleDns(tailscalePrefs->corpDNS);
    settings.exitNodeAllowLanAccess(tailscalePrefs->exitNodeAllowLANAccess);
    settings.advertiseAsExitNode(isExitNode);
    settings.autoUpdateTailscale(tailscalePrefs->autoUpdate_Apply);

    syncSettingsToUi();

    // Make sure current user is operator
    auto isUserOperator = tailscalePrefs->operatorUser == qEnvironmentVariable("USER");
    if (!isUserOperator && !tailscalePrefs->loggedOut) {
        const auto response = QMessageBox::warning(nullptr,
           "Failed to run command",
           "To be able to control tailscale you need to be root or set yourself as operator. Do you want to set yourself as operator?",
           QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

        if (response == QMessageBox::Ok) {
            pCurrentExecution->setOperator();
        }
    }
    else {
        // Operator confirmed, lets continue the flow
        pCurrentExecution->getAccounts();
    }
}

void MainWindow::dnsStatusUpdated(TailDnsStatus* dnsStatus) {
    if (dnsStatus == nullptr)
        return;
    
    pDnsStatus.reset(dnsStatus);

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
    pTrayManager->stateChangedTo(eCurrentState, pTailStatus.get());

    accountsTabUi->onAccountsListed(foundAccounts);
    accountsTabUi->onTailStatusChanged(pTailStatus.get());
}

void MainWindow::onCommandError(const QString& error, bool isSudoRequired) {
    if (isSudoRequired)
    {
        const auto* prefs = pCurrentExecution->currentSettings();
        if (prefs != nullptr && prefs->loggedOut) {
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
}

void MainWindow::loginFlowCompleted() const {
    pCurrentExecution->start();
}

void MainWindow::onIpnEvent(IpnEventData* eventData) {
    if (eventData->Health.Warnings.networkStatus.ImpactsConnectivity) {
        if (eventData->Health.Warnings.networkStatus.WarnableCode == "network-status") {
            if (eCurrentState != TailState::Connected) {
                if (!eventData->Health.Warnings.networkStatus.Text.isEmpty()) {
                    showWarningMessage(eventData->Health.Warnings.networkStatus.Title, eventData->Health.Warnings.networkStatus.Text);
                }
            }
        }
    }
    else {
        if (eventData->Health.Warnings.networkStatus.Severity == "warning") {
            if (!eventData->Health.Warnings.networkStatus.Text.isEmpty())
                showWarningMessage(eventData->Health.Warnings.networkStatus.Title, eventData->Health.Warnings.networkStatus.Text);
        }
        else if (eventData->Health.Warnings.networkStatus.Severity == "error") {
            if (!eventData->Health.Warnings.networkStatus.Text.isEmpty())
                showErrorMessage(eventData->Health.Warnings.networkStatus.Title, eventData->Health.Warnings.networkStatus.Text);
        }
    }

    pCurrentExecution->bootstrap();
}

#if defined(DAVFS_ENABLED)
void MainWindow::drivesListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg) const {
    if (error) {
        pTailStatus->drivesConfigured = false;
        qDebug() << errorMsg;
        qDebug() << "To read more about configuring tail drives, see https://tailscale.com/kb/1369/taildrive";

        QMessageBox::information(nullptr,
            tr("Tail Drive - Error"),
            tr("Tail drives needs to be enabled in ACL. Please go to the admin dashboard.\n\n") + errorMsg,
            QMessageBox::Ok);

        return; // Nothing more to do here
    }

    pTailStatus->drivesConfigured = true;

    // Store available drives
    pTailStatus->drives = QList(drives);

    // Refresh the tray icon menus
    pTrayManager->stateChangedTo(eCurrentState, pTailStatus.get());

    // And the UI for it
    pTailDriveUiManager->stateChangedTo(eCurrentState, pTailStatus.get());
}
#endif

void MainWindow::fileSentToDevice(bool success, const QString& errorMsg, void* userData) const {
    if (!success) {
        pTrayManager->trayIcon()->showMessage(tr("Failed to send file"), errorMsg, QSystemTrayIcon::MessageIcon::Critical, 5000);
    }

    if (userData == nullptr) {
        return;
    }

    auto userDataStr = static_cast<QString*>(userData);
    pTrayManager->trayIcon()->showMessage(tr("File sent"), *userDataStr, QSystemTrayIcon::MessageIcon::Information, 5000);

    // We need to delete this here
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
            pTrayManager->trayIcon()->showMessage(tr("Error"), errorMsg,
                QSystemTrayIcon::MessageIcon::Critical, 5000);
        });
}

void MainWindow::onTailnetFileReceived(QString filePath) const {
    const QFileInfo file(filePath);
    const QString msg("File " + file.fileName() + " was received and saved in " + file.absolutePath());

    pTrayManager->trayIcon()->showMessage(tr("File received"), msg,
        QSystemTrayIcon::MessageIcon::Information, 8000);
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

void MainWindow::netCheckCompleted(bool success, const QMap<QString, QString>& results, QList<QPair<QString, float>>& latencies) const {
    ui->twNetworkStatus->clearContents();
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
        ui->twNetworkStatus->setItem(i, 0, new QTableWidgetItem(key));
        ui->twNetworkStatus->setItem(i, 1, new QTableWidgetItem(value));

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
    ui->twNetworkStatus->setItem(i, 0, new QTableWidgetItem("DERP Latencies"));
    ui->twNetworkStatus->setItem(i, 1, new QTableWidgetItem(""));
    ++i;

    for (auto it = latencies.begin(); it != latencies.end(); ++it) {
        const auto& key = it->first;
        const auto& value = it->second;
        ui->twNetworkStatus->setItem(i, 0, new QTableWidgetItem(key));

        QString val;
        if (value >= 999999)
            val = "-";
        else
            val = QString::number(value) + "ms";

        ui->twNetworkStatus->setItem(i, 1, new QTableWidgetItem(val));
        ++i;
    }
}

void MainWindow::showAdvertiseRoutesDialog() const {
    auto knownRoutes = pCurrentExecution->currentSettings()->getFilteredAdvertiseRoutes();
    
    AdvertiseRoutesDlg dlg(knownRoutes);
    dlg.setWindowIcon(windowIcon());
    auto result = dlg.exec();
    if (result != QDialog::Accepted)
        return;
    
    const auto& routes = dlg.getDefinedRoutes();
    pCurrentExecution->advertiseRoutes(routes);
}

void MainWindow::showDnsSettingsDialog() const {    
    DnsSettingsDlg dlg(pDnsStatus.get(), ui->chkUseTailscaleDns->isChecked());
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

    pTrayManager->trayIcon()->showMessage(title, message, QSystemTrayIcon::MessageIcon::Warning, 5000);
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

    pTrayManager->trayIcon()->showMessage(title, message, QSystemTrayIcon::MessageIcon::Critical, 5000);
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

void MainWindow::showEvent(QShowEvent *event) {
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
        pTailStatus = std::make_unique<TailStatus>();
        pTailStatus->self = std::make_unique<TailDeviceInfo>();
        pTailStatus->user = std::make_unique<TailUser>();
    }

    if (newState == TailState::Connected) {
        setWindowIcon(QIcon(":/icons/tray-on.png"));
        if (didChangeState) {
            seenWarningsAndErrors.clear();
        }
    }
    else {
        setWindowIcon(QIcon(":/icons/tray-off.png"));
    }

    pTrayManager->stateChangedTo(newState, pTailStatus.get());
    accountsTabUi->onTailStatusChanged(pTailStatus.get());

    if (eCurrentState == TailState::NotConnected) {
        // Nothing more to do
        return retVal;
    }

    auto isOnline = eCurrentState == TailState::Connected;
    if (isOnline && retVal != TailState::Connected) {
        startListeningForIncomingFiles();

#if defined(DAVFS_ENABLED)
    pTailDriveUiManager->stateChangedTo(newState, pTailStatus.get());
    if (settings.tailDriveEnabled()) {
        pCurrentExecution->listDrives();
    }
#endif
    }

    return retVal;
}

void MainWindow::onTailStatusChanged(TailStatus* pNewStatus)
{
    // NOTE: Make sure to capture any stored drive data from prev drive listing
    //       if not we will lose track of the drives
    QList<TailDriveInfo> drives;
    if (pTailStatus != nullptr) {
        drives = pTailStatus->drives;
    }
    pTailStatus.reset(pNewStatus);

    // And copy over to new status
    pTailStatus->drives = drives;

    const auto* tailscalePrefs = pCurrentExecution->currentSettings();

    if (pTailStatus->user->id > 0) {
        if (pTailStatus->self->online)
            changeToState(TailState::Connected);
        else
            changeToState(TailState::NotConnected);

        if (pTailStatus->health.count() > 0)
        {
            auto now = QDateTime::currentDateTime();
            QString str{};
            for (const auto& s : pTailStatus->health)
                str += s + "\n";

            if (str.length() > 1)
                showWarningMessage(tr("Warning"), str);
        }

        auto formattedVersion = pTailStatus->version.mid(0, pTailStatus->version.indexOf("-"));
        ui->lblVersionNumber->setText(tr("Version ") + formattedVersion);
    }
    else {
        if (tailscalePrefs != nullptr) {
            if (tailscalePrefs->loggedOut)
                changeToState(TailState::NotLoggedIn);
        }
        else {
            // Assume (fairly safely) that we are not logged in
            changeToState(TailState::NotLoggedIn);
        }
    }

    accountsTabUi->onTailStatusChanged(pTailStatus.get());
    pTailDriveUiManager->stateChangedTo(eCurrentState, pTailStatus.get());
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

    auto advertisedRoutes = pCurrentExecution->currentSettings()->getFilteredAdvertiseRoutes();
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
