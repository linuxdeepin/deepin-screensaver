// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowconfig.h"
#include "slideshowconfigdialog.h"

#include <QDir>
#include <QStandardPaths>
#include <QApplication>

class SlideshowConfigGlobal : public SlideshowConfig
{
};
Q_GLOBAL_STATIC(SlideshowConfigGlobal, slideshowConfig);


SlideshowConfig::SlideshowConfig(QObject *parent)
    : QObject(parent)
{
    m_settings.reset(new QSettings(confPath(), QSettings::IniFormat));
    m_dCfg = DConfig::create(kCfgAppId, kCfgName, "", this);
}

SlideshowConfig::~SlideshowConfig()
{
}

SlideshowConfig *SlideshowConfig::instance()
{
    return slideshowConfig;
}

QString SlideshowConfig::slideshowPath() const
{
    QString path;
    static const QString defaultPath(QDir::homePath() + "/Pictures");

    if (m_dCfg->isValid()) {
        path = m_dCfg->value(kSlideshowPath, defaultPath).toString();
    }
    else {
        m_settings->beginGroup(kGroupSlideshowPath);
        path = m_settings->value(kKeyValue, defaultPath).toString();
        m_settings->endGroup();
    }

    if (path.startsWith(QStringLiteral("~")))
        path.replace(QStringLiteral("~"), QDir::homePath());

    return path;
}

void SlideshowConfig::setSlideShowPath(QString path)
{
    if (path.startsWith(QStringLiteral("~")))
        path.replace(QStringLiteral("~"), QDir::homePath());

    m_settings->beginGroup(kGroupSlideshowPath);
    m_settings->setValue(kKeyValue, path);
    m_settings->endGroup();
}

int SlideshowConfig::intervalTime() const
{
    int intervalTime = 0;
    if (m_dCfg->isValid()) {
        intervalTime = m_dCfg->value(kIntervalTime, kDefaultTime).toInt();
    }
    else {
        m_settings->beginGroup(kGroupIntervalTime);
        intervalTime = m_settings->value(kKeyValue, kDefaultTime).toInt();
        m_settings->endGroup();
    }

    if (intervalTime < kMinIntervalTime || intervalTime > kMaxIntervalTime)
        intervalTime = kDefaultTime;

    return intervalTime;
}

void SlideshowConfig::setIntervalTime(int tempTime)
{
    if (tempTime < kMinIntervalTime || tempTime > kMaxIntervalTime)
        tempTime = kDefaultTime;

    m_settings->beginGroup(kGroupIntervalTime);
    m_settings->setValue(kKeyValue, tempTime);
    m_settings->endGroup();
}

bool SlideshowConfig::isShuffle() const
{
    bool shuffle;
    if (m_dCfg->isValid()) {
        shuffle = m_dCfg->value(kIntervalTime, false).toBool();
    }
    else {
        m_settings->beginGroup(kGroupShuffle);
        shuffle = m_settings->value(kKeyValue, false).toBool();
        m_settings->endGroup();
    }

    return shuffle;
}

void SlideshowConfig::setShuffle(const bool shuffle)
{
    m_settings->beginGroup(kGroupShuffle);
    m_settings->setValue(kKeyValue, shuffle);
    m_settings->endGroup();
}

bool SlideshowConfig::startCustomConfig()
{
    SlideShowConfigDialog *configDialog = new SlideShowConfigDialog();
    dCfgValid = configDialog->dconfigValid();
    if (!dCfgValid)
        return false;

    configDialog->startConfig();

    return true;
}

QString SlideshowConfig::confPath()
{
    static QString confFilePath;

    if (confFilePath.isEmpty()) {
        auto configPaths = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
        Q_ASSERT(!configPaths.isEmpty());

        confFilePath = configPaths.first();
        confFilePath = confFilePath
                + "/" + QApplication::organizationName()
                + "/" + kDeepinScreenSaver + "/"
                + QApplication::applicationName()
                + "/" + QApplication::applicationName() + ".conf";

        QFileInfo confFile(confFilePath);

        // 本地配置文件不存在
        if (!confFile.exists()) {

            QDir confDir = confFile.absoluteDir();
            if (!confDir.exists())
                confDir.mkpath(confDir.absolutePath());

            // 系统配置文件存在，则拷贝。否则不做处理，后面设置值时会自动创建
            QString pathGeneral("/etc");
            pathGeneral = pathGeneral
                    + "/" + kDeepinScreenSaver
                    + "/" + QApplication::applicationName()
                    + "/" + QApplication::applicationName() + ".conf";
            QFile confGeneralFile(pathGeneral);
            if (confGeneralFile.exists()) {
                confGeneralFile.copy(confFilePath);
            }
        }
    }

    return confFilePath;
}
