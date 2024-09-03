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
    path = m_dCfg->value(kSlideshowPath, "").toString();

    // 组策略值为空，使用默认url
    if (path.isEmpty())
        path = defaultPath();

    //path格式修正
    if (path.startsWith(QStringLiteral("https:// ")))
        path = defaultPath();

    return path;
}

void SlideshowConfig::setSlideShowPath(QString path)
{
    m_dCfg->setValue(kSlideshowPath, path);
}

bool SlideshowConfig::startCustomConfig()
{
    auto ins = new SingleInstance(qApp->applicationName() + "-config", this);
    if (ins->isSingle()) {
        SlideShowConfigDialog *configDialog = new SlideShowConfigDialog();

        connect(ins, &SingleInstance::handArguments, configDialog, [configDialog](){
            configDialog->activateWindow();;
        });

        configDialog->startConfig();
    } else {
        ins->sendNewClient();
        return false;
    }

    return true;
}

QString SlideshowConfig::defaultPath()
{
    //用户输入为空时返回
    return "https://uniontech.com";
}
