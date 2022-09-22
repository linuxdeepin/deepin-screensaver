// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>

class Utils
{
public:
    static bool hasConfigSettings(const QString &screenSaverName);
    static QString configLocationPath(const QString &screenSaverName);
    static QString configGeneralPath(const QString &screenSaverName);
    static QString jsonPath(const QString &screenSaverName);
    static bool isWayLand();
    static QList<QDir> allModuleDirList();
};

#endif // UTILS_H
