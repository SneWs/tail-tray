#ifndef TAILUSER_H
#define TAILUSER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

#include <memory>
#include <utility>

#include "JsonHelpers.h"

class TailUser final : public QObject
{
    Q_OBJECT
public:
    long long id = 0;
    QString loginName{};
    QString displayName{};
    QString profilePicUrl{};
    QList<QString> roles{};

    TailUser() {
    }

    TailUser(const TailUser& other)
        : id(other.id)
        , loginName(other.loginName)
        , displayName(other.displayName)
        , profilePicUrl(other.profilePicUrl)
        , roles(other.roles)
    {}

    TailUser& operator = (const TailUser& other) {
        id = other.id;
        loginName = other.loginName;
        displayName = other.displayName;
        profilePicUrl = other.profilePicUrl;
        roles = other.roles;

        return *this;
    }

    static TailUser parse(const QJsonObject& obj, long long userId) {
        auto idKey = QString::number(userId);
        if (!obj.contains(idKey) || obj[idKey].isNull()) {
            return TailUser{};
        }

        // Get the user object based on the user id (user id comes from the self part in the same doc)
        auto thisUser = obj[idKey].toObject();
        return parse(thisUser);
    }

    static TailUser parse(const QJsonObject& thisUser) {
        TailUser user{};

        user.id = jsonReadLong(thisUser, "ID");
        user.loginName = jsonReadString(thisUser, "LoginName");
        user.displayName = jsonReadString(thisUser, "DisplayName");
        user.profilePicUrl = jsonReadString(thisUser, "ProfilePicUrl");

        if (thisUser.contains("roles"))
        {
            for (const auto& ab : thisUser["roles"].toArray()) {
                if (ab.isNull())
                    continue;

                user.roles.emplace_back(ab.toString(""));
            }
        }

        return user;
    }
};

#endif //TAILUSER_H
