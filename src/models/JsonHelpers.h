#ifndef JSONHELPERS_H
#define JSONHELPERS_H

#include <QString>
#include <QJsonObject>

namespace JsonHelpers {
    static QString jsonReadString(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return QString{};

        if (obj[key].isNull())
            return QString{};

        return obj[key].toString("");
    }

    static int jsonReadInt(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return 0;

        if (obj[key].isNull())
            return 0;

        return obj[key].toInt();
    }

    static long long jsonReadLong(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return 0;

        if (obj[key].isNull())
            return 0;

        return obj[key].toInteger();
    }

    static bool jsonReadBool(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return false;

        if (obj[key].isNull())
            return false;

        return obj[key].toBool();
    }
}

#endif // JSONHELPERS_H
