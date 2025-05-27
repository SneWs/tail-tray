#include <QObject>
#include <QTest>

#include "../src/models/Models.h"

class TestTailDeviceInfo : public QObject
{
    Q_OBJECT
private slots:
    void testDeviceInfoCreation() {
        TailDeviceInfo info{};
        QCOMPARE(info.isMullvadExitNode(), false);
    }
};

QTEST_MAIN(TestTailDeviceInfo)
#include "test-device-info.moc"
