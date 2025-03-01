#ifndef NETWORKSTATEMONITOR_H
#define NETWORKSTATEMONITOR_H

#include <QProcess>
#include <QTimer>

#include <memory>

#include "TailRunner.h"

class NetworkStateMonitor : public QObject {
    Q_OBJECT
public:
    explicit NetworkStateMonitor(QObject* parent = nullptr);
    ~NetworkStateMonitor() override = default;

    void shutdown();

signals:
    void netCheckCompleted(bool success, const QMap<QString, QString>& information, QList<QPair<QString, float>>& latencies);

private slots:
    void onTimerElapsed();

    void onProcessCanReadStdOut();
    void onProcessCanReadStandardError();

private:
    void startProcess();

private:
    std::unique_ptr<QProcess> pProcess;
    std::unique_ptr<QTimer> pTimer;

    QMap<QString, QString> information;
    QList<QPair<QString, float>> latencies;
};

#endif //NETWORKSTATEMONITOR_H
