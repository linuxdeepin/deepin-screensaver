// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWCONFIG_H
#define SLIDESHOWCONFIG_H

#include <QObject>
#include <QSettings>
#include <QScopedPointer>
#include <DConfig>

DCORE_USE_NAMESPACE

static const char *const kGroupSlideshowPath = "custom_screensaver.path.select_path";
static const char *const kKeyValue = "value";

static const char *const kDeepinScreenSaver = "deepin-screensaver";

static constexpr char kCfgAppId[] { "org.deepin.screensaver" };
static constexpr char kCfgName[] { "org.deepin.webscreensaver" };

static constexpr char kSlideshowPath[] { "slideshowPath" };

class SlideshowConfig : public QObject
{
    Q_OBJECT
    friend class SlideshowScreenSaver;

public:
    enum PlayMode {
        Order = 0,
        Shuffle = 1
    };

    static SlideshowConfig *instance();

    ~SlideshowConfig();

    QString slideshowPath() const;
    void setSlideShowPath(QString path);

    bool startCustomConfig();
    static QString defaultPath();

protected:
    explicit SlideshowConfig(QObject *parent = nullptr);

private:
    SlideshowConfig(SlideshowConfig &) = delete;
    SlideshowConfig &operator=(SlideshowConfig &) = delete;

private:
    DConfig *m_dCfg;
};

#endif   // SLIDESHOWCONFIG_H
