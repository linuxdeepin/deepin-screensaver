// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"

#include <QFileInfo>
#include <QProcess>
#include <QApplication>
#include <QStandardPaths>

#define CUSTOM_SCREENSAVER_JSON_FILE_PATH "/etc/deepin-screensaver/"

bool Utils::hasConfigSettings(const QString &screenSaverName)
{
    QString jsonFilePath(CUSTOM_SCREENSAVER_JSON_FILE_PATH);
    jsonFilePath += screenSaverName + "/" + screenSaverName + ".json";
    QFileInfo jsonFile(jsonFilePath);
    if (!jsonFile.exists() || !jsonFile.isReadable()) {
        return false;
    }

    return true;
}

QString Utils::configLocationPath(const QString &screenSaverName)
{
    auto paths = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    Q_ASSERT(!paths.isEmpty());

    QString path = paths.first();
    path = path
            + "/" + QApplication::organizationName()
            + "/" + "deepin-screensaver"
            + "/" + screenSaverName
            + "/" + screenSaverName + ".conf";

    return path;
}

QString Utils::configGeneralPath(const QString &screenSaverName)
{
    QString path("/etc");
    path = CUSTOM_SCREENSAVER_JSON_FILE_PATH + screenSaverName
            + "/" + screenSaverName + ".conf";

    return path;
}

QString Utils::jsonPath(const QString &screenSaverName)
{
    QString path("/etc");
    path = CUSTOM_SCREENSAVER_JSON_FILE_PATH + screenSaverName
            + "/" + screenSaverName + ".json";

    return path;
}

bool Utils::isWayLand()
{
    static int wayland = 0;
    static bool isWland = false;

    if (wayland > 0) {
        return isWland;
    } else {
        auto e = QProcessEnvironment::systemEnvironment();
        QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
        QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

        //在wayland平台下设置固定大小，解决属性框最大化问题
        if (XDG_SESSION_TYPE == QLatin1String("wayland") ||
                WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
            isWland = true;
        }
        wayland = 1;
    }
    return isWland;
}
