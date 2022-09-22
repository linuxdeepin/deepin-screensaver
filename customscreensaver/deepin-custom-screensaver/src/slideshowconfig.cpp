// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowconfig.h"

#include <QDir>
#include <QStandardPaths>
#include <QApplication>

class SlideshowConfigGlobal : public SlideshowConfig {};
Q_GLOBAL_STATIC(SlideshowConfigGlobal, slideshowConfig);

static const char *const kGroupSlideshowPath = "custom_screensaver.path.select_path";
static const char *const kGroupIntervalTime = "custom_screensaver.slideshow.time_interval";
static const char *const kGroupShuffle = "custom_screensaver.slideshow.shuffle";
static const char *const kKeyValue = "value";

static const char *const kDeepinScreenSaver = "deepin-screensaver";

static const QList<int> kIntervalTimeList {1, 3, 5, 10, 30, 60}; // minutes
static const int kFefaultTimeIndex = 2;

SlideshowConfig::SlideshowConfig(QObject *parent) : QObject(parent)
{
    m_settings.reset(new QSettings(confPath(), QSettings::IniFormat));
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
    static const QString defaultPath(QDir::homePath() + "/Pictures");
    m_settings->beginGroup(kGroupSlideshowPath);
    QString path = m_settings->value(kKeyValue, defaultPath).toString();
    m_settings->endGroup();
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

int SlideshowConfig::intervalTimeIndex() const
{
    // DSettingDialog中保存的是时间序号，为了兼容，此处也只能是时间序号
    m_settings->beginGroup(kGroupIntervalTime);
    int index = m_settings->value(kKeyValue, kFefaultTimeIndex).toInt();
    m_settings->endGroup();

    if (index < 0 || index >= kIntervalTimeList.count())
        index = kFefaultTimeIndex;

    return index;
}

void SlideshowConfig::setIntervalTimeIndex(int index)
{
    if (index < 0 || index >= kIntervalTimeList.count())
        index = kFefaultTimeIndex;

    m_settings->beginGroup(kGroupIntervalTime);
    m_settings->setValue(kKeyValue, index);
    m_settings->endGroup();
}

int SlideshowConfig::intervalTime() const
{
    int index = intervalTimeIndex();

    int time = kIntervalTimeList.at(index) * 60 * 1000;

    return time;
}

bool SlideshowConfig::isShuffle() const
{
    m_settings->beginGroup(kGroupShuffle);
    bool shuffle =  m_settings->value(kKeyValue, false).toBool();
    m_settings->endGroup();

    return shuffle;
}

void SlideshowConfig::setShuffle(const bool shuffle)
{
    m_settings->beginGroup(kGroupShuffle);
    m_settings->setValue(kKeyValue, shuffle);
    m_settings->endGroup();
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
