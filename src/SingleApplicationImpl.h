#ifndef SINGLEAPPLICATIONIMPL_H
#define SINGLEAPPLICATIONIMPL_H

#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QStandardPaths>

class SingleApplicationImpl final : public QApplication {
Q_OBJECT
public:
    explicit SingleApplicationImpl(int &argc, char **argv)
        : QApplication(argc, argv)
        , singleGuard(singleInstanceLockFilePath())
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
        return singleGuard.tryLock(0);
    }

private:
    static QString singleInstanceLockFilePath() {
        auto lockDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (lockDir.isEmpty())
            lockDir = QDir::tempPath();

        return QDir(lockDir).absoluteFilePath("tail-tray.grenangen.se.lock");
    }

    QLockFile singleGuard;
};

#endif
