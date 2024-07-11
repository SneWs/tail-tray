#include <QDir>
#include <QFile>

#include "MainWindow.h"
#include "AccountsTabUiManager.h"
#include "./ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , accountsTabUi(nullptr)
    , pTrayManager(nullptr)
    , eCurrentState(TailState::NoAccount)
    , pCurrentExecution(nullptr)
    , pTailStatus(nullptr)
{
    ui->setupUi(this);
    accountsTabUi = new AccountsTabUiManager(ui, this);

    pCurrentExecution = new TailRunner(settings, this);
    connect(pCurrentExecution, &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);

    pTrayManager = new TrayMenuManager(settings, pCurrentExecution, this);

    changeToState(TailState::NotLoggedIn);
    pCurrentExecution->checkStatus();

    connect(ui->btnSettingsClose, &QPushButton::clicked,
this, &MainWindow::settingsClosed);

    syncSettingsToUi();
}

MainWindow::~MainWindow()
{
    delete pCurrentExecution;
    delete pTailStatus;
    delete pTrayManager;
    delete ui;
}

void MainWindow::showSettingsTab() {
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::showAccountsTab() {
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::showAboutTab() {
    ui->tabWidget->setCurrentIndex(2);
}

void MainWindow::settingsClosed() {
    syncSettingsFromUi();
    pCurrentExecution->start();
    hide();
}

TailState MainWindow::changeToState(TailState newState)
{
    auto retVal = eCurrentState;
    eCurrentState = newState;

    pTrayManager->stateChangedTo(newState, pTailStatus);

    return retVal;
}

void MainWindow::onTailStatusChanged(TailStatus* pNewStatus)
{
    delete pTailStatus;

    pTailStatus = pNewStatus;

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

    accountsTabUi->onTailStatusChanged(pTailStatus);
}

void MainWindow::syncSettingsToUi() {
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
