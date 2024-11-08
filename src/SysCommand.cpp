#include "SysCommand.h"

#include <QDebug>

void SysCommand::restartTailscaleDaemon() {
    // cmd /c "net stop /y "Service Name" & sc start "Service Name""
    QStringList args;
    args << "-command";
    args << "Restart-Service Tailscale -Force";

    runCommand("powershell", args, false, true);
}

void SysCommand::refreshDns() {
#if defined(WINDOWS_BUILD)
    QStringList args;
    args << "/flushdns";

    runCommand("ipconfig", args, false, true);
#else
    QStringList args;
    args << "restart";
    args << "systemd-resolved";

    runCommand("systemctl", args, false, true);
#endif
}

void SysCommand::makeDir(const QString& path, bool usePkExec) {
    QStringList args;
    args << "-p";
    args << path;

    runCommand("mkdir", args, false, usePkExec);
}

void SysCommand::mountFs(const QString& remote, const QString& path, const QString& fsType, const QString& options, bool usePkExec) {
    QStringList args;
    args << "-t";
    args << fsType;
    args << remote;
    args << path;

    if (!options.isEmpty()) {
        args << "-o";
        args << options;
    }

    runCommand("mount", args, false, usePkExec);
}

QString SysCommand::readFile(const QString &filePath, bool usePkExec) {
    QStringList args;
    args << filePath;

    pProcess = std::make_unique<QProcess>(this);
    if (usePkExec) {
        args.insert(0, "cat");
        pProcess->start("/usr/bin/pkexec", args);
    }
    else {
        pProcess->start("cat", args);
    }

    pProcess->waitForFinished();
    const QByteArray& data = pProcess->readAllStandardOutput();
    return QString(data);
}

void SysCommand::runCommand(const QString& cmd, QStringList args, bool jsonResult, bool usePkExec) {
    pProcess = std::make_unique<QProcess>(this);
    connect(pProcess.get(), &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        qDebug() << "Process exit code " << exitCode << " - " << status;
        emit commandFinished(exitCode);
    });

    if (jsonResult)
        args << "--json";

    if (usePkExec) {
        args.insert(0, cmd);
#if defined(WINDOWS_BUILD)
        // NOTE: Windows 11 24H2 comes with sudo command and needs to be enabled under System > Developer Settings
        pProcess->start("sudo", args);
#else
        pProcess->start("/usr/bin/pkexec", args);
#endif
    }
    else {
        pProcess->start(cmd, args);
    }
}
