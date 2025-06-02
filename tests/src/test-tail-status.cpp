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
                QVERIFY(!peer.location.country.isEmpty());
                QVERIFY(!peer.location.city.isEmpty());
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

        auto mapByCountryAndCity = status.getMullvadExitNodesByCountry();

        QVERIFY(!mapByCountryAndCity.isEmpty());
        QVERIFY(mapByCountryAndCity.contains("Sweden"));
        QVERIFY(mapByCountryAndCity["Sweden"].count() == 3);
        QVERIFY(mapByCountryAndCity["Sweden"].contains("Stockholm"));

        for (const auto& country : mapByCountryAndCity.keys()) {
            const auto& countryMap = mapByCountryAndCity[country];
            for (const auto& city : countryMap.keys()) {
                const auto& peers = countryMap[city];
                QVERIFY(!peers.isEmpty());

                for (const auto& peer : peers) {
                    QVERIFY(peer.isMullvadExitNode());
                    QVERIFY(peer.hasLocationInfo());
                    QVERIFY(!peer.hostName.isEmpty());
                    QVERIFY(!peer.dnsName.isEmpty());
                }
            }
        }
    }

    void testSortingMullvadPeersByDnsName() {
        QFile file("./data/tail-status-mullvad.json");
        file.open(QIODevice::ReadOnly);
        auto json = QJsonDocument::fromJson(file.readAll());
        file.close();

        auto status = TailStatus::parse(json.object());
        QVERIFY(!status.peers.isEmpty());

        auto mapByCountryAndCity = status.getMullvadExitNodesByCountry();

        QVERIFY(!mapByCountryAndCity.isEmpty());
        QVERIFY(mapByCountryAndCity.contains("Sweden"));
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"].count() == 9);

        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][0].dnsName == "se-got-wg-001.mullvad.ts.net.");
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][1].dnsName == "se-got-wg-002.mullvad.ts.net.");
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][2].dnsName == "se-got-wg-003.mullvad.ts.net.");

        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][3].dnsName == "se-got-wg-004.mullvad.ts.net.");
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][4].dnsName == "se-got-wg-005.mullvad.ts.net.");
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][5].dnsName == "se-got-wg-006.mullvad.ts.net.");

        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][6].dnsName == "se-got-wg-007.mullvad.ts.net.");
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][7].dnsName == "se-got-wg-008.mullvad.ts.net.");
        QVERIFY(mapByCountryAndCity["Sweden"]["Gothenburg"][8].dnsName == "se-got-wg-101.mullvad.ts.net.");
    }
};

QTEST_MAIN(TestTailStatus)
#include "test-tail-status.moc"
