#include "TailRunner.h"

#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

TailRunner::TailRunner(QObject* parent)
    : QObject(parent)
    , pProcess(nullptr)
    , eCommand(Command::Status)
{ }

TailRunner::~TailRunner() {
    delete pProcess;
}

void TailRunner::checkStatus() {
    eCommand = Command::Status;
    runCommand("status", QStringList(), true);
}

void TailRunner::start() {
    // tailscale up --operator marcus --accept-routes --exit-node pelican
    eCommand = Command::Connect;
    QStringList args;
    args << "--operator" << "marcus";
    args << "--accept-routes";
    args << "--exit-node" << "pelican";

    runCommand("up", args, true);
}

void TailRunner::stop() {
    eCommand = Command::Disconnect;
    runCommand("down", QStringList());
}

void TailRunner::setUseTailscaleDns(bool use) {
    eCommand = Command::SettingsChange;
    QStringList args;
    if (use) {
        args << "--accept-dns";
    }
    else {
        args << "--accept-dns=false";
    }
    runCommand("set", args);
}

void TailRunner::setAcceptRoutes(bool accept) {
    eCommand = Command::SettingsChange;
    QStringList args;
    if (accept) {
        args << "--accept-routes";
    }
    else {
        args << "--accept-routes=false";
    }
    runCommand("set", args);
}

void TailRunner::allowIncomingConnections(bool allow) {
    eCommand = Command::SettingsChange;
    QStringList args;
    if (allow) {
        args << "--shields-up=false";
    }
    else {
        args << "--shields-up";
    }
    runCommand("set", args, false, true);
}

void TailRunner::setOperator(const QString& username) {
    eCommand = Command::SettingsChange;
    QStringList args;
    if (!username.isEmpty()) {
        qDebug() << "Setting operator to     " << username;
        args << "--operator" << username;
    }
    else {
        args << "--operator";
    }
    runCommand("set", args, false, true);
}

void TailRunner::advertiseAsExitNode(bool enabled) {
    eCommand = Command::SettingsChange;
    QStringList args;
    if (enabled) {
        args << "--advertise-exit-node";
    }
    else {
        args << "--advertise-exit-node=false";
    }
    runCommand("set", args);
}

void TailRunner::exitNodeAllowLanAccess(bool enabled) {
    eCommand = Command::SettingsChange;
    QStringList args;
    if (enabled) {
        args << "--exit-node-allow-lan-access";
    }
    else {
        args << "--exit-node-allow-lan-access=false";
    }

    runCommand("set", args);
}

void TailRunner::useExitNode(const QString  & exitNodeName) {
    //
    eCommand = Command::SettingsChange;
    QStringList args;
    args << "--exit-node";
    if (!exitNodeName.isEmpty()) {
        args << exitNodeName;
    }

    runCommand("set", args);
}

void TailRunner::runCommand(QString cmd, QStringList args, bool jsonResult, bool usePkExec) {
    if (pProcess != nullptr) {
        if (pProcess->state() == QProcess::Running) {
            assert(!"Process already running!");
            return;
        }

        delete pProcess;
    }

    pProcess = new QProcess(this);
    connect(pProcess, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        qDebug() << "Process exit code " << exitCode << " - " << status;

        if (exitCode != 0) {
            if (eCommand == Command::Connect || eCommand == Command::Disconnect) {
                // If we failed to connect or disconnect we probably need to invoke pkexec
                // and change the operator to current user since we won't be able to control it otherwise
                // as a regular user

                auto response = QMessageBox::warning(nullptr,
                    "Failed to run command",
                    "To be able to control tailscale you need to be root or set yourself as operator. Do you want to set yourself as operator?",
                    QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

                if (response == QMessageBox::Ok) {
                    pProcess->close();
                    setOperator(qEnvironmentVariable("USER"));
                }
            }
        }
        else {
            // After we've invoked a command not status command we check for new status update
            if (eCommand != Command::Status) {
                checkStatus();
            }
        }
    });

    connect(pProcess, &QProcess::readyReadStandardOutput,
        this, &TailRunner::onProcessCanReadStdOut);

    connect(pProcess, &QProcess::readyReadStandardError,
        this, [this]() {
            QString errorInfo(pProcess->readAllStandardError());
            if (!errorInfo.isEmpty()) {
                qDebug() << errorInfo;
            }
        });

    if (jsonResult)
        args << "--json";

    args.insert(0, cmd);
    if (usePkExec) {
        args.insert(0, "tailscale");
        pProcess->start("/usr/bin/pkexec", args);
    }
    else {
        pProcess->start("tailscale", args);
    }
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
            // Will be a simple json string with backend state
            QJsonParseError* parseError = nullptr;
            QJsonDocument doc = QJsonDocument::fromJson(data, parseError);

            if (parseError != nullptr)
            {
                qDebug() << parseError->errorString();
                return;
            }
            QJsonObject obj = doc.object();
            if (obj.contains("BackendState")) {
                auto newState = obj["BackendState"].toString();
            }
            break;
        }
        default:
            // Just echo out to console for now
            qDebug() << QString(data);
            break;
    }
}

void TailRunner::parseStatusResponse(const QJsonObject& obj) {
    emit statusUpdated(TailStatus::parse(obj));
}
