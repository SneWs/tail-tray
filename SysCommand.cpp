//
// Created by marcus on 2024-07-18.
//

#include "SysCommand.h"

#include <QDebug>

void SysCommand::refreshDns()
{
    QStringList args;
    args << "restart";
    args << "systemd-resolved";

    runCommand("systemctl", args, false, true);
}

void SysCommand::runCommand(const QString& cmd, QStringList args, bool jsonResult, bool usePkExec)
{
    pProcess = std::make_unique<QProcess>(this);
    connect(pProcess.get(), &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        qDebug() << "Process exit code " << exitCode << " - " << status;
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
