#ifndef TAILRUNNER_H
#define TAILRUNNER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QProcess>
#include <QStringList>

#include "models.h"

class TailRunner : public QObject
{
    Q_OBJECT
public:
    explicit TailRunner(QObject* parent = nullptr);
    virtual ~TailRunner();

    void checkStatus();

    void start();
    void stop();

private:
    QProcess* pProcess;
    enum class Command {
        Connect,
        Disconnect,
        Status
    };

    Command eCommand;

signals:
    void statusUpdated(TailStatus* newStatus);

private:
    void runCommand(QString cmd, QStringList args, bool jsonResult = false);
    void onProcessCanReadStdOut();

    void parseStatusResponse(const QJsonObject& obj);
};

#endif // TAILRUNNER_H
