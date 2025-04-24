#ifndef SINGLEAPPLICATIONIMPL_H
#define SINGLEAPPLICATIONIMPL_H

#include <QApplication>
#include <QSharedMemory>

class SingleApplicationImpl final : public QApplication {
Q_OBJECT
public:
    explicit SingleApplicationImpl(int &argc, char **argv)
        : QApplication(argc, argv)
        , pSingleGuard(new QSharedMemory("tail-tray.grenangen.se/shm", this))
    {
    }

    ~SingleApplicationImpl() override {
        if(pSingleGuard != nullptr && pSingleGuard->isAttached())
            pSingleGuard->detach();
        delete pSingleGuard;
    }

    [[nodiscard]] bool isOwningSingleInstance() const {
        return pSingleGuard != nullptr && pSingleGuard->isAttached();
    }

    [[nodiscard]] bool claimInstance() const {
        if(pSingleGuard->attach(QSharedMemory::ReadOnly)) {
            pSingleGuard->detach();
            return false;
        }

        if(!pSingleGuard->create(1))
            return false;

        return true;
    }

private:
    QSharedMemory* pSingleGuard;
};

#endif
