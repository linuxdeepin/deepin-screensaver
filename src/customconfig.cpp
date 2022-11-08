// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "customconfig.h"
#include "screensaversettingdialog.h"
#include "utils.h"
#include "singleapplication.h"

#include <QApplication>

CustomConfig::CustomConfig(QObject *parent) : QObject(parent)
{
}

CustomConfig::~CustomConfig()
{
    if (m_singleApp) {
        delete m_singleApp;
        m_singleApp = nullptr;
    }

    if (m_settingDialiog) {
        delete m_settingDialiog;
        m_settingDialiog = nullptr;
    }
}

bool CustomConfig::startCustomConfig(const QString &name)
{
    if (!Utils::hasConfigFile(name))
        return false;

    if (!m_singleApp)
        m_singleApp = new SingleApplication();

    if (!m_singleApp->setSingleInstance("ScreenSaverSettingDialog"))
        return true;

    if (m_settingDialiog)
        delete m_settingDialiog;

    m_settingDialiog = new ScreenSaverSettingDialog(name);
    m_settingDialiog->setAttribute(Qt::WA_DeleteOnClose);
    m_settingDialiog->show();
    return true;
}

