#include "MainWindow.h"
#include "./ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pTrayManager(nullptr)
    , eCurrentState(TailState::NoAccount)
    , pCurrentExecution(nullptr)
    , pTailStatus(nullptr)
    , pStatusCheckTimer(nullptr)
{
    ui->setupUi(this);

    pStatusCheckTimer = new QTimer(this);
    connect(pStatusCheckTimer, &QTimer::timeout, this, [this]() {
        pCurrentExecution->checkStatus();
    });
    pStatusCheckTimer->setSingleShot(false);
    pStatusCheckTimer->start(1000 * 30); // 30sec interval

    pCurrentExecution = new TailRunner(settings, this);
    connect(pCurrentExecution, &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);

    pTrayManager = new TrayMenuManager(pCurrentExecution, this);
    connect(pTrayManager->trayIcon(), &QSystemTrayIcon::activated,
        this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
                if (this->isVisible())
                    this->hide();
                else
                        this->show();
            }
            else if (reason == QSystemTrayIcon::ActivationReason::Context) {
                // Restart background status refresh
                pStatusCheckTimer->start();
            }
        }
    );

    changeToState(TailState::NotLoggedIn);
    pCurrentExecution->checkStatus();

    connect(ui->btnSettingsClose, &QPushButton::clicked,
this, &MainWindow::settingsClosed);

    syncSettingsToUi();
}

MainWindow::~MainWindow()
{
    pStatusCheckTimer->stop();
    delete pStatusCheckTimer;

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
    }

    ui->chkRunAsExitNode->setChecked(pTailStatus->self->exitNode);
}

void MainWindow::syncSettingsToUi() {
    ui->chkAllowIncomingCnx->setChecked(settings.allowIncomingConnections());
    ui->chkUseTailscaleDns->setChecked(settings.useTailscaleDns());
    ui->chkUseTailscaleSubnets->setChecked(settings.useSubnets());
    ui->chkRunAsExitNode->setChecked(settings.advertiseAsExitNode());
    ui->chkExitNodeAllowNetworkAccess->setChecked(settings.exitNodeAllowLanAccess());
    ui->chkStartOnLogin->setChecked(settings.startOnLogin());
}

void MainWindow::syncSettingsFromUi() {
    settings.allowIncomingConnections(ui->chkAllowIncomingCnx->isChecked());
    settings.useTailscaleDns(ui->chkUseTailscaleDns->isChecked());
    settings.useSubnets(ui->chkUseTailscaleSubnets->isChecked());
    settings.advertiseAsExitNode(ui->chkRunAsExitNode->isChecked());
    settings.exitNodeAllowLanAccess(ui->chkExitNodeAllowNetworkAccess->isChecked());
    settings.startOnLogin(ui->chkStartOnLogin->isChecked());

    settings.save();
}
