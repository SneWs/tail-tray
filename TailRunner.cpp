#include "TailRunner.h"

#include <QProcess>
#include <QDebug>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

TailRunner::TailRunner(QObject* parent)
    : QObject(parent)
    , pProcess(nullptr)
    , eCommand(Command::Status) {
}

TailRunner::~TailRunner()
{
    delete pProcess;
}

void TailRunner::checkStatus()
{
    eCommand = Command::Status;
    runCommand("status", QStringList(), true);
}

void TailRunner::start()
{
    // tailscale up --operator marcus --accept-routes --exit-node pelican
    eCommand = Command::Connect;
    QStringList args;
    args << "--operator" << "marcus";
    args << "--accept-routes";
    args << "--exit-node" << "pelican";

    runCommand("up", args, true);
}

void TailRunner::stop()
{
    eCommand = Command::Disconnect;
    runCommand("down", QStringList());
}

void TailRunner::runCommand(QString cmd, QStringList args, bool jsonResult)
{
    if (pProcess != nullptr)
    {
        if (pProcess->state() == QProcess::Running) {
            assert(!"Process already running!");
            return;
        }

        delete pProcess;
    }

    pProcess = new QProcess(this);
    connect(pProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        qDebug() << "Failed to run process: " << err;
    });

    connect(pProcess, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        qDebug() << "Process exited with " << exitCode << " - " << status;

        // After we've invoked a command not status command we check for new status update
        if (eCommand != Command::Status) {
            checkStatus();
        }
    });

    connect(pProcess, &QProcess::readyReadStandardOutput,
        this, &TailRunner::onProcessCanReadStdOut);

    if (jsonResult)
        args << "--json";

    args.insert(0, cmd);
    pProcess->start("tailscale", args);
}

void TailRunner::onProcessCanReadStdOut() {
    assert(pProcess != nullptr);

    auto data = pProcess->readAllStandardOutput();

    // Parse the status object

    switch (eCommand) {
        case Command::Status: {
            QJsonParseError* parseError = nullptr;
            QJsonDocument doc = QJsonDocument::fromJson(data, parseError);

            if (parseError != nullptr)
            {
                qDebug() << parseError->errorString();
                return;
            }
            QJsonObject obj = doc.object();
            parseStatusResponse(obj);
            break;
        }
        case Command::Connect: {
            break;
        }
        case Command::Disconnect: {
            break;
        }
    }
}

void TailRunner::parseStatusResponse(const QJsonObject& obj) {
    emit statusUpdated(TailStatus::parse(obj));
}
