#include "TailRunner.h"

#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QByteArray>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

TailRunner::TailRunner(const TailSettings& s, QObject* parent)
: QObject(parent)
    , settings(s)
    , eCommand(Command::Status)
{ }

void TailRunner::checkStatus() {
    eCommand = Command::Status;
    runCommand("status", QStringList(), true);
}

void TailRunner::getAccounts() {
    eCommand = Command::ListAccounts;
    QStringList args;
    args << "--list";
    runCommand("switch", args);
}

void TailRunner::switchAccount(const QString& accountId) {
    eCommand = Command::SwitchAccount;
    QStringList args;
    args << accountId;
    runCommand("switch", args);
}

void TailRunner::login() {
    eCommand = Command::Login;
    QStringList args;
    args << "--operator" << qEnvironmentVariable("USER");

    runCommand("login", args, false, true);
}

void TailRunner::logout() {
    eCommand = Command::Logout;
    QStringList args;
    runCommand("logout", args, false, true);
}

void TailRunner::start(bool usePkExec) {
    eCommand = Command::Connect;
    QStringList args;

    args << "--reset";
    args << "--operator" << qEnvironmentVariable("USER");

    if (settings.useTailscaleDns())
        args << "--accept-dns";
    else
        args << "--accept-dns=false";

    if (settings.acceptRoutes())
        args << "--accept-routes";
    else
        args << "--accept-routes=false";

    if (settings.allowIncomingConnections())
        args << "--shields-up=false";
    else
        args << "--shields-up";

    if (settings.advertiseAsExitNode()) {
        args << "--advertise-exit-node";
    }
    else {
        // Check if we have a exit node that we should use
        auto exitNode = settings.exitNodeInUse();
        if (!exitNode.isEmpty()) {
            qDebug() << "Will use exit node" << exitNode;
            args << "--exit-node" << exitNode;

            if (settings.exitNodeAllowLanAccess())
                args << "--exit-node-allow-lan-access";
            else
                args << "--exit-node-allow-lan-access=false";
        }
        else {

            args << "--exit-node=";
        }
    }

    runCommand("up", args, false, usePkExec);
}

void TailRunner::stop() {
    eCommand = Command::Disconnect;
    runCommand("down", QStringList());
}

void TailRunner::listDrives() {
    eCommand = Command::Drive;

    QStringList args;
    args << "list";
    runCommand("drive", args, false);
}

void TailRunner::addDrive(const TailDriveInfo& drive) {
    eCommand = Command::Drive;

    QStringList args;
    args << "share";
    args << drive.name << drive.path;

    runCommand("drive", args, false);
}

void TailRunner::runCommand(const QString& cmd, QStringList args, bool jsonResult, bool usePkExec) {
    if (pProcess != nullptr) {
        if (pProcess->state() == QProcess::Running) {
            qDebug() << "Process already running!" << "Will queue up " << cmd << args << "command";
            QTimer::singleShot(500, this, [this, cmd, args, jsonResult, usePkExec]() {
                runCommand(cmd, args, jsonResult, usePkExec);
            });
            return;
        }
    }

    pProcess = std::make_unique<QProcess>(this);
    connect(pProcess.get(), &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
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
                    start(true);
                }
            }
        }
        else {
            if (eCommand == Command::SwitchAccount) {
                getAccounts();
            }
            else if (eCommand == Command::ListAccounts) {
                checkStatus();
            }
            else if (eCommand != Command::Status && eCommand != Command::Logout && eCommand != Command::Drive) {
                QTimer::singleShot(1000, this, [this]() {
                    checkStatus();
                });
            }
        }
    });

    connect(pProcess.get(), &QProcess::readyReadStandardOutput,
        this, &TailRunner::onProcessCanReadStdOut);

    // TODO: @grenis This needs some refactoring
    connect(pProcess.get(), &QProcess::readyReadStandardError,
        this, [this]() {
            // NOTE! For whatever reason, the login command output is not captured by the readyReadStandardOutput signal
            // and arrives as a error output, so we need to check for that here.
            if (eCommand == Command::Login) {
                QString message(pProcess->readAllStandardError());
                if (!message.isEmpty()) {
                    qDebug() << message;
                    if (message.startsWith("Success", Qt::CaseSensitivity::CaseInsensitive)) {
                        // Login was successfull
                        // Wait for a bit before triggering flow completed
                        // Flow completed will call start etc
                        QTimer::singleShot(1000, this, [this]() {
                            emit loginFlowCompleted();
                        });
                    }
                    else {
                        QRegularExpression regex(R"(https:\/\/login\.tailscale\.com\/a\/[a-zA-Z0-9]+)");
                        QRegularExpressionMatch match = regex.match(message);
                        if (match.hasMatch()) {
                            QString url = match.captured(0);
                            auto res = QMessageBox::information(nullptr, "Login", "To login you will have to visit " + url + "\n\nPress OK to open the URL",
                                QMessageBox::Ok, QMessageBox::Ok);
                            if (res == QMessageBox::Ok)
                                QDesktopServices::openUrl(QUrl(url));
                        }
                        else {
                            // Failure
                            QMessageBox::warning(nullptr, "Login failure", "Login failed. Message: \n" + message,
                                QMessageBox::Ok, QMessageBox::Ok);
                        }
                    }
                }
            }
            else if (eCommand == Command::Drive) {
                // ACL not allowing drives most likely
                QString errorInfo(pProcess->readAllStandardError());
                emit driveListed(QList<TailDriveInfo>(), true, errorInfo);
            }
            else {
                QString errorInfo(pProcess->readAllStandardError());
                if (!errorInfo.isEmpty()) {
                    qDebug() << errorInfo;
                }
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

    // We need to handle the login by reading as soon as ther is output available since
    // the process will be running until the login flow have completed
    if (eCommand == Command::Login) {
        pProcess->waitForReadyRead();
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
        case Command::ListAccounts: {
            const QString raw(data);
            const QList<TailAccountInfo> accounts = TailAccountInfo::parseAllFound(raw);
            emit accountsListed(accounts);
            break;
        }
        case Command::Drive: {
            const QString raw(data);
            const QList<TailDriveInfo> drives = TailDriveInfo::parse(raw);
            emit driveListed(drives, false, QString());
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
                //auto newState = obj["BackendState"].toString();
            }
            break;
        }
        case Command::Login: {
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
