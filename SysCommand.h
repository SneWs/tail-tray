//
// Created by marcus on 2024-07-18.
//
#ifndef SYSCOMMAND_H
#define SYSCOMMAND_H

#include <QObject>
#include <QProcess>

class SysCommand : public QObject
{
    Q_OBJECT
public:
    void refreshDns();
    void makeDir(const QString& path, bool usePkExec = false);
    void mountFs(const QString& remote, const QString& path, const QString& fsType, const QString& options, bool usePkExec = false);

    int waitForFinished(int msecs = 30000) { return pProcess->waitForFinished(msecs); }

signals:
    void commandFinished(int exitCode);

private:
    std::unique_ptr<QProcess> pProcess;

    void runCommand(const QString& cmd, QStringList args, bool jsonResult = false, bool usePkExec = false);
};

#endif //SYSCOMMAND_H
