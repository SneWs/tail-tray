#include "TailRunner.h"

#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace
{
    static QString commandToString(const Command cmd) {
        switch (cmd) {
            case Command::SetOperator: return "SetOperator";
            case Command::ListAccounts: return "ListAccounts";
            case Command::SwitchAccount: return "SwitchAccount";
            case Command::Login: return "Login";
            case Command::Logout: return "Logout";
            case Command::Connect: return "Connect";
            case Command::Disconnect: return "Disconnect";
            case Command::SettingsChange: return "SettingsChange";
            case Command::Status: return "Status";
            case Command::SendFile: return "SendFile";
            case Command::SetExitNode: return "SetExitNode";
            case Command::GetSettings: return "GetSettings";
            case Command::GetDnsStatus: return "GetDnsStatus";
            case Command::SetSettings: return "SetSettings";
            case Command::AdvertiseRoutes: return "AdvertiseRoutes";
#if defined(DAVFS_ENABLED)
            case Command::Drive: return "Drive";
            case Command::DriveAdd: return "DriveAdd";
            case Command::DriveRename: return "DriveRename";
            case Command::DriveRemove: return "DriveRemove";
#endif
        }

        return "Unknown (Not mapped) command (" + QString::number(static_cast<int>(cmd)) + ")";
    }

}

TailRunner::TailRunner(const TailSettings& s, QObject* parent)
    : QObject(parent)
    , settings(s)
    , processes() {
}

TailRunner::~TailRunner()
{
    runCompletedCleanup();
}

void TailRunner::bootstrap() {
    // NOTE: The bootstrap to get this started is as follows:
    // 1. Read settings from Tailscale daemon
    // 2. Once that is successfully read, it will internally call getAccounts()
    // 3. Once getAccounts() have returned it will once again internally call getStatus()
    // 4. Once getStatus() returns we are in a running state, eg logged in and connected OR logged out OR logged in and disconnected etc...

    readSettings();
}

void TailRunner::readSettings() {
    QStringList args;
    args << "prefs";

    runCommand(Command::GetSettings, "debug", args, false, false);
}

void TailRunner::readDnsStatus() {
    QStringList args;
    args << "status";

    runCommand(Command::GetDnsStatus, "dns", args, false, false);
}

void TailRunner::setOperator() {
    QStringList args;
    args << "--operator=" + qEnvironmentVariable("USER");
    runCommand(Command::SetOperator, "set", args, false, true);
}

void TailRunner::setExitNode(const QString& exitNode) {
    QStringList args;
    args << "--exit-node=" + (exitNode.isEmpty() ? "" : exitNode);
    runCommand(Command::SetExitNode, "set", args, false, false);
}

void TailRunner::advertiseRoutes(const QList<QString>& definedRoutes) {
    QStringList args;
    args << "--advertise-routes";
    if (definedRoutes.count() > 0) {
        args << definedRoutes.join(",");
    }
    else {
        // Empty resets the routes
        args << "";
    }

    runCommand(Command::AdvertiseRoutes, "set", args, false, false);
}

void TailRunner::applySettings(const TailSettings& s) {
    QStringList args;
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

        // Check if we have an exit node that we should use
        if (settings.exitNodeAllowLanAccess())
            args << "--exit-node-allow-lan-access";
        else
            args << "--exit-node-allow-lan-access=false";
    }
    else {
        args << "--advertise-exit-node=false";
        args << "--exit-node-allow-lan-access=false";
    }

    qDebug() << "TailRunner::applySettings: " << args;

    runCommand(Command::SetSettings, "set", args, false, false);
}

void TailRunner::checkStatus() {
    runCommand(Command::Status, "status", QStringList(), true);
}

void TailRunner::getAccounts() {
    QStringList args;
    args << "--list";
    runCommand(Command::ListAccounts, "switch", args);
}

void TailRunner::switchAccount(const QString& accountId) {
    QStringList args;
    args << accountId;
    runCommand(Command::SwitchAccount, "switch", args);
}

void TailRunner::login() {
    QStringList args;
#if !defined(WINDOWS_BUILD)
    args << "--operator" << qEnvironmentVariable("USER");
#endif

    runCommand(Command::Login, "login", args, false, true);
}

void TailRunner::logout() {
    runCommand(Command::Logout, "logout", QStringList(), false, true);
}

void TailRunner::start(const bool usePkExec) {
    applySettings(settings);

    QStringList args;
    runCommand(Command::Connect, "up", args, false, usePkExec);
}

void TailRunner::stop() {
    runCommand(Command::Disconnect, "down", QStringList());
}

#if defined(DAVFS_ENABLED)
void TailRunner::listDrives() {
    QStringList args;
    args << "list";
    runCommand(Command::Drive, "drive", args, false);
}

void TailRunner::addDrive(const TailDriveInfo& drive) {
    QStringList args;
    args << "share";
    args << drive.name << drive.path;

    runCommand(Command::DriveAdd, "drive", args, false);
}

void TailRunner::renameDrive(const TailDriveInfo &drive, const QString &newName) {
    QStringList args;
    args << "rename";
    args << drive.name << newName;

    runCommand(Command::DriveRename, "drive", args, false);
}

void TailRunner::removeDrive(const TailDriveInfo& drive) {
    QStringList args;
    args << "unshare";
    args << drive.name;

    runCommand(Command::DriveRemove, "drive", args, false);
}
#endif

void TailRunner::sendFile(const QString& targetDevice, const QString& localFilePath, void* userData) {
    QStringList args;
    args << "cp";
    args << localFilePath;
    args << targetDevice + ":";

    runCommand(Command::SendFile, "file", args, false, false, userData);
}

void TailRunner::runCommand(const Command cmdType, const QString& cmd, const QStringList& args, const bool jsonResult, const bool usePkExec, void* userData) {
    auto wrapper = new BufferedProcessWrapper(cmdType, cmdType == Command::Login, this);
    processes.emplace_back(wrapper);

    connect(wrapper, &BufferedProcessWrapper::processFinished,
        this, &TailRunner::onProcessFinished);

    connect(wrapper, &BufferedProcessWrapper::processCanReadStdOut,
        this, &TailRunner::onProcessCanReadStdOut);

    connect(wrapper, &BufferedProcessWrapper::processCanReadStandardError,
        this, &TailRunner::onProcessCanReadStandardError);

    wrapper->start(cmd, args, jsonResult, usePkExec, userData);

    // We need to handle the login by reading as soon as there is output available since
    // the process will be running until the login flow have completed
    if (cmdType == Command::Login) {
        wrapper->process()->waitForReadyRead();
    }
}

void TailRunner::onProcessCanReadStdOut(const BufferedProcessWrapper* wrapper) {
    const auto data = wrapper->process()->readAllStandardOutput();

    // Parse the status object
    switch (wrapper->command()) {
        case Command::Status: {
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError)
            {
                qDebug() << parseError.errorString();
                return;
            }

            const QJsonObject obj = doc.object();
            parseStatusResponse(obj);
            break;
        }
        case Command::GetSettings: {
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError)
            {
                qDebug() << parseError.errorString();
                return;
            }

            const QJsonObject obj = doc.object();
            parseSettingsResponse(obj);
            break;
        }
        case Command::GetDnsStatus: {
            QString raw(data);
            if (raw.isEmpty()) {
                qDebug() << "No DNS status found on stdout or stderr";
                emit commandError("No DNS status found", false);

            }
            else {
                auto dnsStatus = TailDnsStatus::parse(raw);
                if (dnsStatus == nullptr) {
                    qDebug() << "Failed to parse DNS status";
                    emit commandError("Failed to parse DNS status", false);
                }
                else {
                    emit dnsStatusRead(dnsStatus);
                }
            }
            break;
        }
        case Command::ListAccounts: {
            const QString raw(data);
            const QList<TailAccountInfo> accounts = TailAccountInfo::parseAllFound(raw);
            emit accountsListed(accounts);
            break;
        }
        case Command::Connect: {
            // Will be a simple json string with backend state
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError)
            {
                qDebug() << parseError.errorString();
                return;
            }

            const QJsonObject obj = doc.object();
            if (obj.contains("BackendState")) {
                //auto newState = obj["BackendState"].toString();
            }
            break;
        }
#if defined(DAVFS_ENABLED)
        case Command::Drive: {
            const QString raw(data);
            const QList<TailDriveInfo> drives = TailDriveInfo::parse(raw);
            emit driveListed(drives, false, QString());
        }
#endif
        default:
            // Just echo out to console for now
            qDebug() << QString(data);
            break;
    }
}

void TailRunner::onProcessCanReadStandardError(const BufferedProcessWrapper* wrapper) {
    const auto commandInfo = wrapper->command();

    // NOTE! For whatever reason, the login command output is not captured by the readyReadStandardOutput signal
    // and arrives as a error output, so we need to check for that here.
    if (commandInfo == Command::Login) {
        const QString message(wrapper->process()->readAllStandardError());
        if (!message.isEmpty()) {
            qDebug() << message;
            if (message.startsWith("Success", Qt::CaseSensitivity::CaseInsensitive)) {
                // Login was successful
                // Wait for a bit before triggering flow completed
                //  will call start etc
                QTimer::singleShot(1000, this, [this]() {
                    emit loginFlowCompleted();
                });
            }
            else {
                static QRegularExpression regex(R"(https:\/\/login\.tailscale\.com\/a\/[a-zA-Z0-9]+)");
                const QRegularExpressionMatch match = regex.match(message);
                if (match.hasMatch()) {
                    const QString url = match.captured(0);
                    const auto res = QMessageBox::information(nullptr, "Login", "To login you will have to visit " + url + "\n\nPress OK to open the URL",
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
#if defined(DAVFS_ENABLED)
    else if (commandInfo == Command::Drive) {
        // ACL not allowing drives most likely
        const QString errorInfo(wrapper->process()->readAllStandardError());
        emit driveListed(QList<TailDriveInfo>(), true, errorInfo);
    }
#endif
    else {
        const QString errorInfo(wrapper->process()->readAllStandardError());
        if (!errorInfo.isEmpty()) {
            qDebug() << errorInfo;

            // Scan the output and look for sudo indicators
            // If detected, it means we need to elevate to complete the command and/or
            // the user hasn't set itself as operator yet
            static QRegularExpression regex(R"(sudo\s+tailscale(?:\s+\S*)?)");
            const QRegularExpressionMatch match = regex.match(errorInfo);
            if (match.hasMatch()) {
                emit commandError(errorInfo, true);
            }
        }
    }
}

void TailRunner::onProcessFinished(const BufferedProcessWrapper* process, int exitCode, const QProcess::ExitStatus exitStatus) {
    qDebug() << "Process exit code " << exitCode << " - " << exitStatus;

    // Cleanup processes that has completed, this will not include this current process that invoked the signal
    // as it's not flagged as completed until after we return from this call
    runCompletedCleanup();

    const auto commandInfo = process->command();
    if (exitCode != 0) {
        if (commandInfo == Command::Connect || commandInfo == Command::Disconnect) {
            // If we failed to connect or disconnect we probably need to invoke pkexec
            // and change the operator to current user since we won't be able to control it otherwise
            // as a regular user

            const QString info(process->process()->readAllStandardOutput());
            if (info.contains("https://login.tailscale.com")) {
                qDebug() << "Login required!";
                static QRegularExpression regex(R"(https:\/\/login\.tailscale\.com\/a\/[a-zA-Z0-9]+)");
                const QRegularExpressionMatch match = regex.match(info);
                if (match.hasMatch()) {
                    const QString url = match.captured(0);
                    const auto res = QMessageBox::information(nullptr, "Login", "To login you will have to visit " + url + "\n\nPress OK to open the URL",
                        QMessageBox::Ok, QMessageBox::Ok);

                    if (res == QMessageBox::Ok)
                        QDesktopServices::openUrl(QUrl(url));
                }
            }
            else {
                bool isUserOperator = false;
                if (currentPrefs != nullptr) {
                    isUserOperator = currentPrefs->operatorUser == qEnvironmentVariable("USER");
                }

                qDebug() << "Failed to execute. Is current user operator? " << (isUserOperator ? "Yes" : "No");
            }
        }
        else if (commandInfo == Command::SendFile) {
            emit fileSent(false, QString(process->process()->readAllStandardError()), process->userData());
        }
    }
    else {
        if (commandInfo == Command::SwitchAccount || commandInfo == Command::Login) {
            getAccounts();
        }
        else if (commandInfo == Command::GetSettings) {
            emit settingsRead();
        }
        else if (commandInfo == Command::Status) {
            readDnsStatus();
        }
        else if (commandInfo == Command::SetExitNode) {
            readSettings();
            checkStatus();
        }
        else if (commandInfo == Command::AdvertiseRoutes ||
                 commandInfo == Command::SetSettings) {
            readSettings();
        }
        else if (commandInfo == Command::ListAccounts) {
            checkStatus();
        }
        else if (commandInfo == Command::SendFile) {
            emit fileSent(true, QString{}, process->userData());
        }
        else if (commandInfo != Command::Status && commandInfo != Command::Logout &&
                 commandInfo != Command::GetDnsStatus
#if defined(DAVFS_ENABLED)
                 && commandInfo != Command::Drive
#endif
            )
        {
            QTimer::singleShot(1000, this, [this]() {
                checkStatus();
            });
        }
    }
}

void TailRunner::parseStatusResponse(const QJsonObject& obj) {
    emit statusUpdated(TailStatus::parse(obj));
}

void TailRunner::parseSettingsResponse(const QJsonObject& obj) {
    currentPrefs = std::move(CurrentTailPrefs::parse(obj));
}

bool TailRunner::hasPendingCommandOfType(const Command cmdType) const {
    for (const auto* process : processes) {
        if (process->command() == cmdType && !process->isCompleted())
            return true;
    }

    return false;
}

void TailRunner::runCompletedCleanup() {
    for (auto it = processes.begin(); it != processes.end();) {
        if ((*it)->isCompleted()) {
            const auto cmd = commandToString((*it)->command());
            qDebug() << "Cleaning up process " << cmd;

            (*it)->deleteLater();
            it = processes.erase(it);
        }
        else {
            ++it;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ProcessWrapper impl

BufferedProcessWrapper::BufferedProcessWrapper(const Command cmd, bool emitOnActualSignals, QObject* parent)
    : QObject(parent)
    , bEmitOnActualSignals(emitOnActualSignals)
    , proc(std::make_unique<QProcess>(this))
    , pUserData(nullptr)
    , eCommand(cmd)
    , completed(false)
    , didReceiveStdErr(false)
    , didReceiveStdOut(false)
{
    connect(proc.get(), &QProcess::finished,
            this, &BufferedProcessWrapper::onProcessFinished);

    connect(proc.get(), &QProcess::readyReadStandardOutput,
            this, &BufferedProcessWrapper::onProcessCanReadStdOut);

    connect(proc.get(), &QProcess::readyReadStandardError,
            this, &BufferedProcessWrapper::onProcessCanReadStandardError);
}

void BufferedProcessWrapper::start(const QString& cmd, QStringList args, const bool jsonResult, const bool usePkExec, void* userData) {
    didReceiveStdErr = false;
    didReceiveStdOut = false;
    pUserData = userData;

    if (jsonResult)
        args << "--json";

    args.insert(0, cmd);

#if !defined(WINDOWS_BUILD)
    if (usePkExec) {
        args.insert(0, "tailscale");
        proc->start("/usr/bin/pkexec", args);
    }
    else {
        proc->start("tailscale", args);
    }
#else
    // Windows don't have pkexec etc and we don't need to set operator
    proc->start("tailscale", args);
#endif
}

void BufferedProcessWrapper::onProcessCanReadStdOut() {
    didReceiveStdOut = true;
    if (bEmitOnActualSignals)
        emit processCanReadStdOut(this);
}

void BufferedProcessWrapper::onProcessCanReadStandardError() {
    didReceiveStdErr = true;
    if (bEmitOnActualSignals)
        emit processCanReadStandardError(this);
}

void BufferedProcessWrapper::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (didReceiveStdOut)
        emit processCanReadStdOut(this);

    if (didReceiveStdErr)
        emit processCanReadStandardError(this);

    emit processFinished(this, exitCode, exitStatus);

    // Should always be after emitting returns
    completed = true;
}
