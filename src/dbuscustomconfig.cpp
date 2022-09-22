// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbuscustomconfig.h"
#include "screensaversettingdialog.h"
#include "utils.h"

#include <QApplication>

DBusCustomConfig::DBusCustomConfig(QObject *parent) : QObject(parent)
{
    m_autoQuitTimer.setInterval(30000);
    m_autoQuitTimer.setSingleShot(true);
#ifndef QT_DEBUG
    m_autoQuitTimer.start();
#endif

    connect(&m_autoQuitTimer, &QTimer::timeout, this, [this]{
        if (!m_settingDialiog) {
            qApp->quit();
        }
    });
}

DBusCustomConfig::~DBusCustomConfig()
{
    if (m_settingDialiog) {
        delete m_settingDialiog;
        m_settingDialiog = nullptr;
    }
}

void DBusCustomConfig::StartCustomConfig(const QString &name)
{
    if (name.isEmpty() || !Utils::hasConfigSettings(name))
        return;

    if (m_settingDialiog) {
        delete m_settingDialiog;
    }

    m_settingDialiog = new ScreenSaverSettingDialog(name);
    m_settingDialiog->setAttribute(Qt::WA_DeleteOnClose);
    m_settingDialiog->show();
}

QStringList DBusCustomConfig::ListWithCustomConfigurations()
{
    QStringList screenSaverList;
    QList<QDir> m_moduleDirList {QDir("://deepin-screensaver/modules"), QDir(MODULE_PATH)};
    m_moduleDirList.prepend(QDir(QString("%1/.local/lib/%2/modules").arg(QDir::homePath(), qApp->applicationName())));

    for (const QDir &moduleDir : m_moduleDirList) {
        for (const QFileInfo &info : moduleDir.entryInfoList(QDir::Files)) {
            if (!info.isExecutable() && info.suffix() != "qml")
                continue;

            if (Utils::hasConfigSettings(info.fileName()))
                screenSaverList.append(info.fileName());
        }
    }

    return screenSaverList;
}

bool DBusCustomConfig::HasConfigSettings(const QString &name)
{
    if (name.isEmpty())
        return false;
    return Utils::hasConfigSettings(name);
}

