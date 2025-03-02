#ifndef TAILDRIVEINFO_H
#define TAILDRIVEINFO_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class TailDriveInfo final : public QObject
{
    Q_OBJECT
public:
    QString name;
    QString path;
    QString as;

    explicit TailDriveInfo(QString n = QString(), QString p = QString(), QString a = QString())
        : QObject()
        , name(std::move(n))
        , path(std::move(p))
        , as(std::move(a))
    {}

    TailDriveInfo(const TailDriveInfo& o)
        : QObject()
        , name(o.name)
        , path(o.path)
        , as(o.as)
    {}

    TailDriveInfo& operator = (const TailDriveInfo& o) {
        name = o.name;
        path = o.path;
        as = o.as;

        return *this;
    }

    static QList<TailDriveInfo> parse(const QString& rawString) {
        QList<TailDriveInfo> parsedList{};
        static QRegularExpression re(R"(^(\S+)\s+([\S\\/:\.]+)\s*(\S*)\s*$)");

        QStringList lines = rawString.split('\n');
        if (lines.count() < 3) {
            // Just headers, no data
            return parsedList;
        }

        // Skip first 2 lines as it's headers only
        for (int i = 2; i < lines.count(); i++) {
            const QString& line = lines[i];
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                TailDriveInfo data;
                data.name = match.captured(1);
                data.path = match.captured(2);
                data.as = match.captured(3);
                parsedList.emplace_back(data);
            }
        }

        return parsedList;
    }
};

#endif // TAILDRIVEINFO_H
