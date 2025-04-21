#ifndef TAILDNSSTATUS_H
#define TAILDNSSTATUS_H

#include <QObject>
#include <QList>
#include <QRegularExpression>
#include <QString>

#include <memory>


class TailDnsStatus final : public QObject
{
    Q_OBJECT
public:
    QList<QPair<QString, QString>> splitDnsRoutes{};
    QList<QString> searchDomains{};

    TailDnsStatus() {
    }

    TailDnsStatus(const TailDnsStatus& other) {
        splitDnsRoutes = other.splitDnsRoutes;
        searchDomains = other.searchDomains;
    }

    TailDnsStatus& operator = (const TailDnsStatus& other) {
        splitDnsRoutes = other.splitDnsRoutes;
        searchDomains = other.searchDomains;

        return *this;
    }

    static TailDnsStatus parse(const QString rawOutput) {
        TailDnsStatus newStatus{};

        // Regular expression to match Split DNS Routes
        static QRegularExpression splitDnsRegex(R"(Split DNS Routes:\s*((?:\s*-\s*\S+\s*->\s*\S+\s*)+))");
        QRegularExpressionMatch splitDnsMatch = splitDnsRegex.match(rawOutput);

        if (splitDnsMatch.hasMatch()) {
            QString splitDnsData = splitDnsMatch.captured(1);
            static QRegularExpression entryRegex(R"(\s*-\s*([\S]+)\s*->\s*([\S]+))");
            QRegularExpressionMatchIterator it = entryRegex.globalMatch(splitDnsData);
            while (it.hasNext()) {
                QRegularExpressionMatch entryMatch = it.next();
                if (entryMatch.captured(1).endsWith("ts.net."))
                    continue;
                newStatus.splitDnsRoutes.emplace_back(qMakePair(entryMatch.captured(1), entryMatch.captured(2)));
            }
        }

        // Regular expression to match Search Domains
        static QRegularExpression searchDomainRegex(R"(Search Domains:\s*((?:\s*-\s*\S+\s*)+))");
        QRegularExpressionMatch searchDomainMatch = searchDomainRegex.match(rawOutput);

        if (searchDomainMatch.hasMatch()) {
            QString searchDomainData = searchDomainMatch.captured(1);
            static QRegularExpression entryRegex(R"(\s*-\s*([\S]+))");
            QRegularExpressionMatchIterator it = entryRegex.globalMatch(searchDomainData);
            while (it.hasNext()) {
                QRegularExpressionMatch entryMatch = it.next();
                newStatus.searchDomains.emplace_back(entryMatch.captured(1));
            }
        }

        return newStatus;
    }
};

#endif /* TAILDNSSTATUS_H */
