#ifndef JSONHELPERS_H
#define JSONHELPERS_H

#include <QString>
#include <QJsonObject>

namespace JsonHelpers {
    static QString safeReadStr(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return QString{};

        if (obj[key].isNull())
            return QString{};

        return obj[key].toString("");
    }

    static int safeReadInt(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return 0;

        if (obj[key].isNull())
            return 0;

        return obj[key].toInt();
    }

    static long long safeReadLong(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return 0;

        if (obj[key].isNull())
            return 0;

        return obj[key].toInteger();
    }

    static bool safeReadBool(const QJsonObject& obj, const QString& key) {
        if (!obj.contains(key))
            return false;

        if (obj[key].isNull())
            return false;

        return obj[key].toBool();
    }
}

#endif // JSONHELPERS_H
