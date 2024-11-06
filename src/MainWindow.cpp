#include <QDir>
#include <QFile>

#include "MainWindow.h"

#include <QList>
#include <QFileDialog>
#include <QMessageBox>

#include "ManageDriveWindow.h"
#include "KnownValues.h"

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
{
    ui->setupUi(this);

    // Remove the tail drive tab by default
    ui->tabWidget->removeTab(2);

    // Make sure to adjust tail drive based on the check state
    if (settings.tailDriveEnabled()) {
        ui->tabWidget->insertTab(3, ui->tabTailDrive, QIcon::fromTheme("drive-removable-media"), "Tail Drive");
    }

    connect(ui->btnAddTailDrive, &QPushButton::clicked,
        this, &MainWindow::addTailDriveButtonClicked);

    connect(ui->btnRemoveSelectedTailDrive, &QPushButton::clicked,
        this, &MainWindow::removeTailDriveButtonClicked);

    connect(ui->chkUseTailDrive, &QCheckBox::clicked,
        this, [this]() {
            auto checked = ui->chkUseTailDrive->isChecked();
            settings.tailDriveEnabled(checked);

            if (checked) {
                ui->tabWidget->insertTab(3, ui->tabTailDrive, QIcon::fromTheme("drive-removable-media"), "Tail Drive");
            }
            else {
                ui->tabWidget->removeTab(3);
            }
    });

    connect(ui->btnSelectTailDriveMountPath, &QPushButton::clicked,
        this, &MainWindow::selectTailDriveMountPath);

    if (!isTailDriveFileAlreadySetup()) {
        ui->btnTailDriveFixDavFsMountSetup->setEnabled(true);
        ui->btnTailDriveFixDavFsMountSetup->setText("Fix it for me");
        connect(ui->btnTailDriveFixDavFsMountSetup, &QPushButton::clicked,
                this, &MainWindow::fixTailDriveDavFsSetup);
    }
    else {
        ui->btnTailDriveFixDavFsMountSetup->setEnabled(false);
        ui->btnTailDriveFixDavFsMountSetup->setText("Configured and ready");
    }

    connect(ui->btnSelectTailFileDefaultSaveLocation, &QPushButton::clicked,
        this, &MainWindow::onShowTailFileSaveLocationPicker);

    pCurrentExecution = std::make_unique<TailRunner>(settings, this);
    connect(pCurrentExecution.get(), &TailRunner::accountsListed, this, &MainWindow::onAccountsListed);
    connect(pCurrentExecution.get(), &TailRunner::commandError, this, &MainWindow::onCommandError);
    connect(pCurrentExecution.get(), &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);
    connect(pCurrentExecution.get(), &TailRunner::loginFlowCompleted, this, &MainWindow::loginFlowCompleted);
    connect(pCurrentExecution.get(), &TailRunner::driveListed, this, &MainWindow::drivesListed);
    connect(pCurrentExecution.get(), &TailRunner::fileSent, this, &MainWindow::fileSentToDevice);

    connect(pNetworkStateMonitor.get(), &NetworkStateMonitor::netCheckCompleted, this, &MainWindow::netCheckCompleted);

    accountsTabUi = std::make_unique<AccountsTabUiManager>(ui.get(), pCurrentExecution.get(), this);
    pTrayManager = std::make_unique<TrayMenuManager>(settings, pCurrentExecution.get(), this);

    changeToState(TailState::NotLoggedIn);
    pCurrentExecution->getAccounts();

    connect(ui->btnSettingsClose, &QPushButton::clicked,
this, &MainWindow::settingsClosed);

    syncSettingsToUi();

    // Make sure the settings tab is selected by default
    ui->tabWidget->setCurrentIndex(1);

    setupNetworkCallbacks();
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
        const auto response = QMessageBox::warning(this, "Sudo required",
            error + "\n\nTo use Tail Tray you need to be set as operator. Do you want to set yourself as operator now?",
            QMessageBox::Ok | QMessageBox::Cancel);

        if (response == QMessageBox::Ok)
            pCurrentExecution->setOperator();

        return;
    }

    QMessageBox::warning(this, "Error running tailscale", error);
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

void MainWindow::onNetworkReachabilityChanged(QNetworkInformation::Reachability newReachability) {
    qDebug() << "onNetworkReachabilityChanged -> " << newReachability;

    if (newReachability == QNetworkInformation::Reachability::Online) {
        // Fetch accounts and then get the status info after that
        pCurrentExecution->getAccounts();
        QTimer::singleShot(500, this, [this]() {
            pCurrentExecution->checkStatus();
        });

        return;
    }

    if (eCurrentState != TailState::NotConnected)
        changeToState(TailState::NotConnected);
}

void MainWindow::drivesListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg) const {
    if (error) {
        pTailStatus->drivesConfigured = false;
        qDebug() << errorMsg;
        qDebug() << "To read more about configuring taill drives, see https://tailscale.com/kb/1369/taildrive";

        QMessageBox::information(nullptr,
            "Tail Drive - Error",
            "Tail drives needs to be enabled in ACL. Please go to the admin dashboard.\n\n" + errorMsg,
            QMessageBox::Ok);

        return; // Nothing more to do here
    }

    pTailStatus->drivesConfigured = true;

    // Store available drives
    pTailStatus->drives = drives;

    // Refresh the tray icon menus
    pTrayManager->stateChangedTo(eCurrentState, pTailStatus.get());

    tailDrivesToUi();
}

void MainWindow::addTailDriveButtonClicked() const {
    ManageDriveWindow dlg(TailDriveInfo{}, nullptr);
    auto result = dlg.exec();
    if (result == QDialog::Accepted) {
        auto newDrive = dlg.driveInfo();
        pCurrentExecution->addDrive(newDrive);

        pTailStatus->drives.emplace_back(newDrive);
        tailDrivesToUi();
    }
}

void MainWindow::removeTailDriveButtonClicked() const {
    auto selectedItems = ui->twSharedDrives->selectedItems();
    if (selectedItems.count() < 1) {
        return;
    }

    auto row = ui->twSharedDrives->row(selectedItems.first());
    const auto& drive = pTailStatus->drives[row];

    auto answer = QMessageBox::question(nullptr,
        "Are you sure?",
        "Do you really want to remove the share " + drive.path + "?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (answer != QMessageBox::Yes) {
        return;
    }

    pCurrentExecution->removeDrive(drive);
    ui->twSharedDrives->removeRow(row);
    if (row > 0)
        ui->twSharedDrives->selectRow(row - 1);
}

void MainWindow::selectTailDriveMountPath() const {
    QFileDialog dlg(nullptr, "Select mount path", ui->txtTailDriveDefaultMountPath->text());
    dlg.setOption(QFileDialog::Option::ShowDirsOnly, true);
    dlg.setFileMode(QFileDialog::FileMode::Directory);

    auto result = dlg.exec();
    if (result == QDialog::Accepted) {
        auto files = dlg.selectedFiles();
        if (files.count() > 0) {
            ui->txtTailDriveDefaultMountPath->setText(files.first());
        }
    }
}

void MainWindow::fixTailDriveDavFsSetup() const {
    auto homeDir = KnownValues::getHomeDir();
    auto homeDavFsSecret = KnownValues::getTailDriveFilePath();

    if (isTailDriveFileAlreadySetup())
        return;

    // Create the .davfs2 folder if it doesn't exist
    auto davDir = QDir(homeDir);
    (void)davDir.mkpath(".davfs2"); // Don't care for return val

    QFile davFsSecret(homeDavFsSecret);
    davFsSecret.open(QIODevice::ReadWrite);

    // We need to add our config lines
    davFsSecret.seek(davFsSecret.size());
    davFsSecret.write(QString("\n# Tailscale davfs server config\n").toUtf8());
    davFsSecret.write(QString(KnownValues::tailDavFsUrl + "\tGuest\tGuest\n").toUtf8());

    davFsSecret.close();

    QMessageBox::information(nullptr, "Tail Tray", "davfs2 config has been written");

    ui->btnTailDriveFixDavFsMountSetup->setEnabled(isTailDriveFileAlreadySetup());
}

void MainWindow::fileSentToDevice(bool success, const QString& errorMsg, void* userData) const {
    if (!success) {
        pTrayManager->trayIcon()->showMessage("Failed to send file", errorMsg, QSystemTrayIcon::MessageIcon::Critical, 5000);
    }

    if (userData == nullptr) {
        return;
    }

    auto userDataStr = static_cast<QString*>(userData);
    pTrayManager->trayIcon()->showMessage("File sent", *userDataStr, QSystemTrayIcon::MessageIcon::Information, 5000);

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
            pTrayManager->trayIcon()->showMessage("Error", errorMsg,
                QSystemTrayIcon::MessageIcon::Critical, 5000);
        });
}

void MainWindow::onTailnetFileReceived(QString filePath) const {
    const QFileInfo file(filePath);
    const QString msg("File " + file.fileName() + " was received and saved in " + file.absolutePath());

    pTrayManager->trayIcon()->showMessage("File received", msg,
        QSystemTrayIcon::MessageIcon::Information, 8000);
}

void MainWindow::onShowTailFileSaveLocationPicker() {
    QFileDialog dlg(this, "Select folder", ui->txtTailFilesDefaultSavePath->text());
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
    ui->twNetworkStatus->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
    ui->twNetworkStatus->setRowCount(static_cast<int>(results.count() + latencies.count() + 1));

    int i = 0;
    for (auto it = results.begin(); it != results.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        ui->twNetworkStatus->setItem(i, 0, new QTableWidgetItem(key));
        ui->twNetworkStatus->setItem(i, 1, new QTableWidgetItem(value));
        ++i;
    }

    // Add DERP header
    ui->twNetworkStatus->setItem(i, 0, new QTableWidgetItem("DERP Latencies"));
    ui->twNetworkStatus->setItem(i, 1, new QTableWidgetItem(""));
    ++i;

    // For latencies, we want to sort on lowest latencies first
    // Sort the list based on the second element (the value)
    std::sort(latencies.begin(), latencies.end(), [](const QPair<QString, float>& a, const QPair<QString, float>& b) {
        return a.second < b.second;
    });

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

void MainWindow::tailDrivesToUi() const {
    if (pTailStatus == nullptr) {
        return;
    }

    const auto& drives = pTailStatus->drives;

    ui->twSharedDrives->clearContents();
    ui->twSharedDrives->setColumnCount(2);
    ui->twSharedDrives->setHorizontalHeaderLabels(QStringList() << "Name" << "Path");
    ui->twSharedDrives->setRowCount(static_cast<int>(drives.count()));

    for (int i = 0; i < drives.count(); i++) {
        const auto& drive = drives[i];
        qDebug() << "Drive: " << drive.name << " (" << drive.path << ")";

        ui->twSharedDrives->setItem(i, 0, new QTableWidgetItem(drive.name));
        ui->twSharedDrives->setItem(i, 1, new QTableWidgetItem(drive.path));
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    // Getting accounts will also trigger a fetch of status
    pCurrentExecution->getAccounts();
}

TailState MainWindow::changeToState(TailState newState)
{
    auto retVal = eCurrentState;
    eCurrentState = newState;

    if (eCurrentState == TailState::NotLoggedIn)
    {
        // Clear the status
        pTailStatus = std::make_unique<TailStatus>();
        pTailStatus->self = std::make_unique<TailDeviceInfo>();
        pTailStatus->user = std::make_unique<TailUser>();
    }

    pTrayManager->stateChangedTo(newState, pTailStatus.get());
    accountsTabUi->onTailStatusChanged(pTailStatus.get());

    auto isOnline = eCurrentState == TailState::Connected;
    if (isOnline && retVal != TailState::Connected) {
        startListeningForIncomingFiles();

        if (settings.tailDriveEnabled()) {
            pCurrentExecution->listDrives();
        }
    }

    return retVal;
}

void MainWindow::onTailStatusChanged(TailStatus* pNewStatus)
{
    pTailStatus.reset(pNewStatus);

    if (pTailStatus->user->id > 0)
    {
        if (pTailStatus->self->online)
            changeToState(TailState::Connected);
        else
            changeToState(TailState::NotConnected);

        if (pTailStatus->health.count() > 0)
        {
            QDateTime now = QDateTime::currentDateTime();
            QString str;
            for (const auto& s : pTailStatus->health)
            {
                if (seenWarnings.contains(s))
                {
                    auto lastSeen = seenWarnings[s];
                    auto daysSinceLastSeen = lastSeen.secsTo(now);
                    if (daysSinceLastSeen < (60 * 60) * 6) // 6 hours before showing the same health info again
                        continue; // No point in showing

                    // Update to now
                    seenWarnings[s] = now;
                }
                else
                {
                    seenWarnings.insert(s, now);
                }

                str += s + "\n";
            }

            if (str.length() > 1)
            {
                pTrayManager->trayIcon()->showMessage("Warning",
                  str,
                  QSystemTrayIcon::Warning, 5000);
            }
        }

        auto formattedVersion = pTailStatus->version.mid(0, pTailStatus->version.indexOf("-"));
        ui->lblVersionNumber->setText("Version " + formattedVersion);
    }
    else {
        changeToState(TailState::NotLoggedIn);
    }

    accountsTabUi->onTailStatusChanged(pTailStatus.get());

    tailDrivesToUi();
}

bool MainWindow::shallowCheckForNetworkAvailable() {
    auto* inst = QNetworkInformation::instance();
    if (inst->reachability() == QNetworkInformation::Reachability::Online)
        return true;

    return false;
}

void MainWindow::setupNetworkCallbacks() const {
    auto* inst = QNetworkInformation::instance();
    if (inst == nullptr) {
        if (!QNetworkInformation::loadDefaultBackend()) {
            qDebug() << "Unable to load Network information stack";
            return;
        }

        inst = QNetworkInformation::instance();
        if (inst == nullptr) {
            qDebug() << "No noetwork information stack available";
            return;
        }
    }

    connect(inst, &QNetworkInformation::reachabilityChanged,
        this, &MainWindow::onNetworkReachabilityChanged);
}

void MainWindow::syncSettingsToUi() const {
    ui->chkAllowIncomingCnx->setChecked(settings.allowIncomingConnections());
    ui->chkUseTailscaleDns->setChecked(settings.useTailscaleDns());
    ui->chkUseTailscaleSubnets->setChecked(settings.useSubnets());
    ui->chkRunAsExitNode->setChecked(settings.advertiseAsExitNode());
    ui->chkExitNodeAllowNetworkAccess->setEnabled(settings.advertiseAsExitNode());
    ui->chkExitNodeAllowNetworkAccess->setChecked(settings.exitNodeAllowLanAccess());
    ui->chkStartOnLogin->setChecked(settings.startOnLogin());
    ui->chkStartOnLogin->setChecked(false);
    ui->chkUseTailDrive->setChecked(settings.tailDriveEnabled());
    ui->txtTailDriveDefaultMountPath->setText(settings.tailDriveMountPath());
    ui->txtTailFilesDefaultSavePath->setText(settings.tailFilesDefaultSavePath());

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
    settings.useSubnets(ui->chkUseTailscaleSubnets->isChecked());
    settings.advertiseAsExitNode(ui->chkRunAsExitNode->isChecked());
    settings.exitNodeAllowLanAccess(ui->chkExitNodeAllowNetworkAccess->isChecked());
    settings.startOnLogin(ui->chkStartOnLogin->isChecked());
    settings.tailDriveEnabled(ui->chkUseTailDrive->isChecked());
    settings.tailDriveMountPath(ui->txtTailDriveDefaultMountPath->text().trimmed());

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
            QFile::copy("/usr/local/share/applications/tail-tray.desktop", targetFile);
        }
    }
    else {
        QFile::remove(targetFile);
    }

    settings.save();
}
