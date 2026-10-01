#ifndef SINGLEAPPLICATIONIMPL_H
#define SINGLEAPPLICATIONIMPL_H

#include <QApplication>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>

class SingleApplicationImpl final : public QApplication {
Q_OBJECT
public:
    explicit SingleApplicationImpl(int &argc, char **argv)
        : QApplication(argc, argv)
        , singleGuard(runtimeFilePath("tail-tray.grenangen.se.lock"))
    {
        singleGuard.setStaleLockTime(0);
    }

    ~SingleApplicationImpl() override {
        if(singleGuard.isLocked())
            singleGuard.unlock();
    }

    [[nodiscard]] bool isOwningSingleInstance() const {
        return singleGuard.isLocked();
    }

    [[nodiscard]] bool claimInstance() {
        if (!singleGuard.tryLock(0))
            return false;

        // Later launches connect here to ask this instance to show its window. Holding the lock
        // means an existing socket is a crashed instance's leftover, which would fail listen().
        QLocalServer::removeServer(activationSocketPath());
        activationServer.listen(activationSocketPath());
        connect(&activationServer, &QLocalServer::newConnection, this, [this]() {
            activationServer.nextPendingConnection()->deleteLater();
            emit activationRequested();
        });

        return true;
    }

    static void activateRunningInstance() {
        QLocalSocket socket;
        socket.connectToServer(activationSocketPath());
        socket.waitForConnected(1000);
    }

signals:
    void activationRequested();

private:
    static QString runtimeFilePath(const QString& fileName) {
        auto runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (runtimeDir.isEmpty())
            runtimeDir = QDir::tempPath();

        return QDir(runtimeDir).absoluteFilePath(fileName);
    }

    static QString activationSocketPath() {
        return runtimeFilePath("tail-tray.grenangen.se.socket");
    }

    QLockFile singleGuard;
    QLocalServer activationServer;
};

#endif
