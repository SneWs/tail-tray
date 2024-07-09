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

    pCurrentExecution = new TailRunner(this);
    connect(pCurrentExecution, &TailRunner::statusUpdated, this, &MainWindow::onTailStatusChanged);

    pTrayManager = new TrayMenuManager(pCurrentExecution, this);

    changeToState(TailState::NotLoggedIn);
    pCurrentExecution->checkStatus();
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
}
