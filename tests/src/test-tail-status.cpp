#include <QObject>
#include <QTest>
#include <QList>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <iostream>

#include "../../src/models/Models.h"

class TestTailStatus : public QObject
{
    Q_OBJECT
private slots:
    void testParsingSucceeds() {
        QFile file("./data/tail-status-mullvad.json");
        file.open(QIODevice::ReadOnly);
        auto json = QJsonDocument::fromJson(file.readAll());
        file.close();

        auto status = TailStatus::parse(json.object());
        QVERIFY(!status.version.isEmpty());
        QVERIFY(!status.self.id.isEmpty());
        QVERIFY(!status.tailscaleIPs.isEmpty());
    }

    void testParsingMullvadExtras() {
        QFile file("./data/tail-status-mullvad.json");
        file.open(QIODevice::ReadOnly);
        auto json = QJsonDocument::fromJson(file.readAll());
        file.close();

        auto status = TailStatus::parse(json.object());
        QVERIFY(!status.peers.isEmpty());
        for (const auto& peer : status.peers) {
            if (peer.exitNodeOption && peer.isMullvadExitNode()) {
                QVERIFY(peer.hasLocationInfo());
                QVERIFY(!peer.getShortDnsName().isEmpty());
            }
        }
    }

    void testSortingMullvadPeersByCountry() {
        QFile file("./data/tail-status-mullvad.json");
        file.open(QIODevice::ReadOnly);
        auto json = QJsonDocument::fromJson(file.readAll());
        file.close();

        auto status = TailStatus::parse(json.object());
        QVERIFY(!status.peers.isEmpty());

        QString output{};
        auto mapByCountryAndCity = status.getMullvadExitNodesByCountry();

        QVERIFY(!mapByCountryAndCity.isEmpty());
        QVERIFY(mapByCountryAndCity.contains("Sweden"));
        QVERIFY(mapByCountryAndCity["Sweden"].count() == 3);
        QVERIFY(mapByCountryAndCity["Sweden"].contains("Stockholm"));

        for (const auto& country : mapByCountryAndCity.keys()) {
            const auto& countryMap = mapByCountryAndCity[country];
            output += QString("Country: %1\n").arg(country);
            for (const auto& city : countryMap.keys()) {
                output += QString("  City: %1\n").arg(city);
                const auto& peers = countryMap[city];
                for (const auto& peer : peers) {
                    output += QString("    Peer ID: %1, DNS: %2\n")
                                  .arg(peer.id, peer.dnsName);
                }
            }
        }

        std::cout << output.toStdString() << std::endl;
    }
};

QTEST_MAIN(TestTailStatus)
#include "test-tail-status.moc"
