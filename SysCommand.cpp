//
// Created by marcus on 2024-07-18.
//

#include "SysCommand.h"

#include <QDebug>

void SysCommand::restartTailscaleDaemon() {
    QStringList args;
    args << "restart";
    args << "tailscaled";

    runCommand("systemctl", args, false, true);
}

void SysCommand::refreshDns() {
    QStringList args;
    args << "restart";
    args << "systemd-resolved";

    runCommand("systemctl", args, false, true);
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

bool SysCommand::copyFile(const QString& fromFile, const QString& toFile, bool usePkExec) {
    QStringList args;
    args << fromFile;
    args << toFile;

    pProcess = std::make_unique<QProcess>(this);
    if (usePkExec) {
        args.insert(0, "cp");
        pProcess->start("/usr/bin/pkexec", args);
    }
    else {
        pProcess->start("cp", args);
    }

    pProcess->waitForFinished();
    qDebug() << QString(pProcess->readAllStandardError());
    return pProcess->exitCode() == 0;
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
        pProcess->start("/usr/bin/pkexec", args);
    }
    else {
        pProcess->start(cmd, args);
    }
}
