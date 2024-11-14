#ifndef TAILACCOUNTINFO_H
#define TAILACCOUNTINFO_H

#include <QObject>
#include <QRegularExpression>
#include <QString>

class TailAccountInfo final : public QObject
{
    Q_OBJECT
public:
    QString id;
    QString tailnet;
    QString account;

    TailAccountInfo()
        : QObject(nullptr)
        , id("")
        , tailnet("")
        , account("")
    { }

    TailAccountInfo(const TailAccountInfo& o)
        : QObject(nullptr)
        , id(o.id)
        , tailnet(o.tailnet)
        , account(o.account)
    { }

    TailAccountInfo& operator = (const TailAccountInfo& o) {
        id = o.id;
        tailnet = o.tailnet;
        account = o.account;

        return *this;
    }

    static TailAccountInfo parse(const QString& rawLineData) {
        // NOTE: This is a raw line from the command output
        // The format is:
        // ID    Tailnet             Account
        static const QRegularExpression regex(R"((\w{4})\s+(\S+)\s+(\S+))");
        TailAccountInfo retVal;
        QRegularExpressionMatch match = regex.match(rawLineData);
        if (match.hasMatch()) {
            retVal.id = match.captured(1);
            retVal.tailnet = match.captured(2);
            retVal.account = match.captured(3);
        }

        return retVal;
    }

    // This handles fragmented lines as well from the command line output
    // so even if a line starts with the ID and then break it will make sure to parse it correctly
    static QList<TailAccountInfo> parseAllFound(const QString& rawData) {
        QList<TailAccountInfo> retVal;

        // Split input into individual lines
        const QStringList lines = rawData.split('\n', Qt::SkipEmptyParts);

        QStringList entries;
        QString currentEntry;

        // Reconstruct broken lines into complete entries
        // And we skip the first line since it's the header line
        for (int i = 1; i < lines.length(); i++) {
            const QString& line = lines[i].trimmed();
            if (isIdLine(line)) {
                if (!currentEntry.isEmpty()) {
                    entries.append(currentEntry);
                    currentEntry.clear();
                }
                currentEntry = line;
            }
            else {
                currentEntry += "\t" + line.trimmed();
            }
        }

        if (!currentEntry.isEmpty()) {
            entries.append(currentEntry);
        }

        // Iterate over each reconstructed entry and apply the regular expression
        for (const QString& entry : entries) {
            qDebug() << "Parsing entry: " << entry;
            retVal.emplace_back(TailAccountInfo::parse(entry));
        }

        return retVal;
    }

    static bool isIdLine(const QString& line) {
        // Check if the line starts with a 4-character word
        static const QRegularExpression idRe(R"(^\w{4}\s+)");
        return idRe.match(line).hasMatch();
    }
};

#endif //TAILACCOUNTINFO_H
