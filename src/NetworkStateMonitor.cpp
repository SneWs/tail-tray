#include "NetworkStateMonitor.h"

#include <QString>
#include <QRegularExpression>

namespace {
    struct NetworkReportResult {
        QMap<QString, QString> information;
        QList<QPair<QString, float>> latencies;
    };

    NetworkReportResult parseReport(const QString& report) {
        NetworkReportResult result;

        // Regex patterns for different sections of the report
        static QRegularExpression keyValueRegex(R"(\*\s*(.+?):\s*(.*))");
        static QRegularExpression derpLatencyRegex(R"(-\s*(\w{3}):\s*([0-9.]+)\s*ms\s*\((.*?)\))");

        // Split the report into lines
        QStringList lines = report.split('\n', Qt::SkipEmptyParts);

        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();

            // Match key-value pairs (lines starting with *)
            QRegularExpressionMatch match = keyValueRegex.match(trimmedLine);
            if (match.hasMatch()) {
                QString key = match.captured(1).trimmed();
                QString value = match.captured(2).trimmed();

                if (value.isEmpty())
                    continue;

                result.information.insert(key, value);
            }

            // Match DERP latency entries (lines starting with -)
            QRegularExpressionMatch derpMatch = derpLatencyRegex.match(trimmedLine);
            if (derpMatch.hasMatch()) {
                QString latencyValue = derpMatch.captured(2).trimmed();  // Capture only the float value (no "ms")
                QString location = derpMatch.captured(3).trimmed();

                if (latencyValue.isEmpty()) {
                    result.latencies.emplace_back(std::move(QPair<QString, float>(location, 999999.0f)));
                }
                else {
                    auto ok = false;
                    const float value = latencyValue.toFloat(&ok);
                    if (!ok)
                        qDebug() << "Failed to convert" << latencyValue << " to float!";

                    result.latencies.emplace_back(std::move(QPair<QString, float>(location,
                        value)));
                }
            }
        }

        return result;
    }
}

NetworkStateMonitor::NetworkStateMonitor(QObject* parent)
    : QObject(parent)
    , pProcess(nullptr)
    , pTimer(nullptr)
{
    pProcess = std::make_unique<QProcess>(this);

    // Check network state every 5min.
    // NOTE: We run an early first single shot timer, that will then adjust the timeout once done
    pTimer = std::make_unique<QTimer>(this);
    pTimer->setSingleShot(false);
    pTimer->setTimerType(Qt::TimerType::CoarseTimer);

    connect(pTimer.get(), &QTimer::timeout, this, &NetworkStateMonitor::onTimerElapsed);
    pTimer->start();

    // Run at startup as well, but with a slight delay of a second
    QTimer::singleShot(1000, this, &NetworkStateMonitor::onTimerElapsed);
}

void NetworkStateMonitor::shutdown() {
    if (pTimer != nullptr) {
        pTimer->stop();
    }

    if (pProcess != nullptr) {
        pProcess->close();
    }
}

void NetworkStateMonitor::onTimerElapsed() {
    startProcess();

    // Start a new 2min interval before next check
    pTimer->setInterval((1000 * 60) * 2);
    pTimer->start();
}

void NetworkStateMonitor::onProcessCanReadStdOut() {
    const QString message(pProcess->readAllStandardOutput());
    auto [newInformation, newLatencies] = parseReport(message);

    // Append/update prev entries
    for (auto it = newInformation.begin(); it != newInformation.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();

        if (value.isEmpty())
            continue;

        information[key] = value;
    }

    for (auto it = newLatencies.begin(); it != newLatencies.end(); ++it) {
        const auto& key = it->first;
        const auto& value = it->second;

        auto didUpdate = false;
        for (int i = 0; i < latencies.size(); ++i) {
            auto& latency = latencies[i];
            if (latency.first == key) {
                didUpdate = true;
                latency.second = value;
                break;
            }
        }

        if (!didUpdate) {
            QPair<QString, float> pair(key, value);
            latencies.emplace_back(std::move(pair));
        }
    }

    emit netCheckCompleted(true, information, latencies);

    // qDebug() << "Network check completed";
    // for (auto it = information.begin(); it != information.end(); ++it) {
    //     const auto& key = it.key();
    //     const auto& value = it.value();
    //     qDebug() << key << ": " << value;
    // }
}

void NetworkStateMonitor::onProcessCanReadStandardError() {
    const QString message(pProcess->readAllStandardError());
    // qDebug() << "NetworkStateMonitor::StdError - " << message;

    emit netCheckCompleted(false, information, latencies);
}

void NetworkStateMonitor::startProcess() {
    if (pProcess != nullptr) {
        pProcess->kill();
        pProcess->disconnect();
        
        pProcess.reset();
    }

    pProcess = std::make_unique<QProcess>(this);

    connect(pProcess.get(), &QProcess::readyReadStandardOutput,
        this, &NetworkStateMonitor::onProcessCanReadStdOut);

    connect(pProcess.get(), &QProcess::readyReadStandardError,
        this, &NetworkStateMonitor::onProcessCanReadStandardError);

    pProcess->start("tailscale", QStringList() << "netcheck");
}

