#include <QDir>
#include <QFile>

#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , accountsTabUi(nullptr)
    , pTrayManager(nullptr)
    , eCurrentState(TailState::NoAccount)
    , pCurrentExecution(nullptr)
    , pTailStatus(nullptr)
{
    ui->setupUi(this);

    pCurrentExecution = std::make_unique<TailRunner>(settings, this);
    connect(pCurrentExecution.get(), &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);
    connect(pCurrentExecution.get(), &TailRunner::loginFlowCompleted, this, &MainWindow::loginFlowCompleted);
    connect(pCurrentExecution.get(), &TailRunner::accountsListed, this, &MainWindow::onAccountsListed);

    accountsTabUi = std::make_unique<AccountsTabUiManager>(ui.get(), pCurrentExecution.get(), this);
    pTrayManager = std::make_unique<TrayMenuManager>(settings, pCurrentExecution.get(), this);

    changeToState(TailState::NotLoggedIn);
    pCurrentExecution->getAccounts();

    connect(ui->btnSettingsClose, &QPushButton::clicked,
this, &MainWindow::settingsClosed);

    syncSettingsToUi();

    // Make sure the settings tab is selected by default
    ui->tabWidget->setCurrentIndex(1);
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

void MainWindow::settingsClosed() {
    syncSettingsFromUi();
    pCurrentExecution->start();
    hide();
}

void MainWindow::loginFlowCompleted() const {
    pCurrentExecution->start();
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

    return retVal;
}

void MainWindow::onTailStatusChanged(TailStatus* pNewStatus)
{
    pTailStatus.reset(pNewStatus);

    if (pTailStatus->user->id > 0)
    {
        // Logged in
        if (pTailStatus->health.count() < 1)
            changeToState(TailState::Connected);
        else
            changeToState(TailState::NotConnected);

        auto formattedVersion = pTailStatus->version.mid(0, pTailStatus->version.indexOf("-"));
        ui->lblVersionNumber->setText("Version " + formattedVersion);
    }

    accountsTabUi->onTailStatusChanged(pTailStatus.get());
}

void MainWindow::syncSettingsToUi() const {
    ui->chkAllowIncomingCnx->setChecked(settings.allowIncomingConnections());
    ui->chkUseTailscaleDns->setChecked(settings.useTailscaleDns());
    ui->chkUseTailscaleSubnets->setChecked(settings.useSubnets());
    ui->chkRunAsExitNode->setChecked(settings.advertiseAsExitNode());
    ui->chkExitNodeAllowNetworkAccess->setChecked(settings.exitNodeAllowLanAccess());
    ui->chkStartOnLogin->setChecked(settings.startOnLogin());
    ui->chkStartOnLogin->setChecked(false);

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
