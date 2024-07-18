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

private:
    std::unique_ptr<QProcess> pProcess;

    void runCommand(const QString& cmd, QStringList args, bool jsonResult = false, bool usePkExec = false);
};

#endif //SYSCOMMAND_H
