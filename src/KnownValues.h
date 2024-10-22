//
// Created by marcus on 2024-07-29.
//

#ifndef KNOWNVALUES_H
#define KNOWNVALUES_H

#include <QString>

namespace KnownValues {
    inline QString tailDavFsUrl("http://100.100.100.100:8080");

    inline QString getHomeDir() {
        return qEnvironmentVariable("HOME");
    }

    inline QString getTailDriveFilePath() {
        auto homeDir = getHomeDir();
        auto homeDavFsDir = homeDir.append("/.davfs2");
        return homeDavFsDir.append("/secrets");
    }

}

#endif //KNOWNVALUES_H
