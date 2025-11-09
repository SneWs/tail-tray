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
            case Command::CheckIfInstalled: return "CheckIfInstalled";
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

    static BufferedProcessWrapper* pActiveLoginFlow = nullptr;
}

TailRunner::TailRunner(const TailSettings& s, QObject* parent)
    : QObject(parent)
    , settings(s)
    , currentPrefs()
    , lastKnownStatus()
{ }

void TailRunner::shutdown() {
    // Kill off any pending process calls, this is needed in cases such as:
    // - When the user is in a pending login state, then the process is waiting for user to complete web auth flows
    cancelLoginFlow();
}

void TailRunner::checkIfInstalled() {
    runCommand(Command::CheckIfInstalled, "--version", QStringList());
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

#if defined(WINDOWS_BUILD)
    if (settings.autoUpdateTailscale())
        args << "--auto-update";
    else
        args << "--auto-update=false";
#endif

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

    //qDebug() << "TailRunner::applySettings: " << args;

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

void TailRunner::cancelLoginFlow() {
    if (pActiveLoginFlow == nullptr) {
        return;
    }

    pActiveLoginFlow->cancel();
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
    listDrives();
}

void TailRunner::renameDrive(const TailDriveInfo &drive, const QString &newName) {
    QStringList args;
    args << "rename";
    args << drive.name << newName;

    runCommand(Command::DriveRename, "drive", args, false);
    listDrives();
}

void TailRunner::removeDrive(const TailDriveInfo& drive) {
    QStringList args;
    args << "unshare";
    args << drive.name;

    runCommand(Command::DriveRemove, "drive", args, false);
    listDrives();
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
    auto isLoginCmd = cmdType == Command::Login;
    if (isLoginCmd && pActiveLoginFlow != nullptr) {
        // Already in a login flow...
        return;
    }

    auto wrapper = new BufferedProcessWrapper(cmdType, isLoginCmd, this);
    if (isLoginCmd) {
        pActiveLoginFlow = wrapper;
    }

    connect(wrapper, &BufferedProcessWrapper::processErrorOccurred,
        this, &TailRunner::onProcessErrorOccurred);

    connect(wrapper, &BufferedProcessWrapper::processFinished,
        this, &TailRunner::onProcessFinished);

    connect(wrapper, &BufferedProcessWrapper::processCanReadStdOut,
        this, &TailRunner::onProcessCanReadStdOut);

    connect(wrapper, &BufferedProcessWrapper::processCanReadStandardError,
        this, &TailRunner::onProcessCanReadStandardError);

    wrapper->start(cmd, args, jsonResult, usePkExec, userData);
}

void TailRunner::onProcessCanReadStdOut(BufferedProcessWrapper* wrapper) {
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
        case Command::CheckIfInstalled: {
            auto hasData = data.size() > 0;
            emit tailscaleIsInstalled(hasData);
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
                emit dnsStatusRead(dnsStatus);
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
            break;
        }
#endif
        default:
            // Just echo out to console for now
            //qDebug() << QString(data);
            break;
    }
}

void TailRunner::onProcessCanReadStandardError(BufferedProcessWrapper* wrapper) {
    const auto commandInfo = wrapper->command();

    // NOTE! For whatever reason, the login command output is not captured by the readyReadStandardOutput signal
    // and arrives as a error output, so we need to check for that here.
    if (commandInfo == Command::Login) {
        const QString message(wrapper->process()->readAllStandardError());
        if (!message.isEmpty()) {
            //qDebug() << message;
            if (message.startsWith("Success", Qt::CaseSensitivity::CaseInsensitive)) {
                // Login was successful
                // Wait for a bit before triggering flow completed
                //  will call start etc
                QTimer::singleShot(1000, this, [this]() {
                    emit loginFlowCompleted(true);
                });
            }
            else {
                static QRegularExpression regex(R"(https:\/\/login\.tailscale\.com\/a\/[a-zA-Z0-9]+)");
                const QRegularExpressionMatch match = regex.match(message);
                if (match.hasMatch()) {
                    const QString url = match.captured(0);
                    QTimer::singleShot(500, this, [this, url]() {
                        emit loginFlowStarting(url);
                    });
                }
                else {
                    emit loginFlowCompleted(false);
                }
            }
        }
    }
    else if (commandInfo == Command::CheckIfInstalled) {
        emit tailscaleIsInstalled(false);
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

        qDebug() << "Command" << commandToString(commandInfo) << "recv. std error";
    }
}

void TailRunner::onProcessErrorOccurred(BufferedProcessWrapper* wrapper, QProcess::ProcessError error) {
    const auto commandInfo = wrapper->command();
    qDebug() << "Command" << commandToString(commandInfo) << "failed to execute!";
    qDebug() << "Command error" << error;

    if (commandInfo == Command::CheckIfInstalled) {
        emit tailscaleIsInstalled(false);
    }
}

void TailRunner::onProcessFinished(BufferedProcessWrapper* process, int exitCode, const QProcess::ExitStatus exitStatus) {
    qDebug() << "Process exit code " << exitCode << " - " << exitStatus;

    process->deleteLater();

    const auto commandInfo = process->command();

    // Reset login flow tracking ptr
    if (commandInfo == Command::Login) {
        pActiveLoginFlow = nullptr;
    }

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
                isUserOperator = currentPrefs.operatorUser == qEnvironmentVariable("USER");

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
    }
}

void TailRunner::parseStatusResponse(const QJsonObject& obj) {
    auto newStatus = TailStatus::parse(obj);
    auto oldStatus = lastKnownStatus;
    lastKnownStatus = newStatus;

    emit statusUpdated(newStatus);

    // If this is startup or a full refresh etc. we do not diff
    if (oldStatus.peers.length() < 1) {
        return;
    }

    // Diff peers and track newly added peers
    for (const auto& peer : lastKnownStatus.peers) {
        const auto existsFromBefore = oldStatus.containsPeer(peer);
        if (!existsFromBefore) {
            // New peer added
            qDebug() << "New peer discovered" << peer.getShortDnsName() << " / " << peer.dnsName;
            emit newPeerDiscovered(peer);
        }
    }

    // Diff peers and track removed peers
    for (const auto& peer : oldStatus.peers) {
        const auto stillExists = lastKnownStatus.containsPeer(peer);
        if (!stillExists) {
            // Peer removed
            qDebug() << "Peer removed" << peer.getShortDnsName() << " / " << peer.dnsName;
            emit peerRemoved(peer);
        }
    }
}

void TailRunner::parseSettingsResponse(const QJsonObject& obj) {
    currentPrefs = CurrentTailPrefs::parse(obj);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ProcessWrapper impl

BufferedProcessWrapper::BufferedProcessWrapper(const Command cmd, bool emitOnActualSignals, QObject* parent)
    : QObject(parent)
    , bEmitOnActualSignals(emitOnActualSignals)
    , proc(std::make_unique<QProcess>(this))
    , pUserData(nullptr)
    , eCommand(cmd)
    , didReceiveStdErr(false)
    , didReceiveStdOut(false)
{
    connect(proc.get(), &QProcess::finished,
            this, &BufferedProcessWrapper::onProcessFinished);

    connect(proc.get(), &QProcess::readyReadStandardOutput,
            this, &BufferedProcessWrapper::onProcessCanReadStdOut);

    connect(proc.get(), &QProcess::errorOccurred,
        this, &BufferedProcessWrapper::onProcessErrorOccurred);

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

void BufferedProcessWrapper::onProcessErrorOccurred(QProcess::ProcessError error) {
    emit processErrorOccurred(this, error);
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
}
