// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWCONFIG_H
#define SLIDESHOWCONFIG_H

#include <QObject>
#include <QSettings>
#include <QScopedPointer>

class SlideshowConfig : public QObject
{
    Q_OBJECT

public:
    static SlideshowConfig *instance();

    ~SlideshowConfig();

    QString slideshowPath() const;
    void setSlideShowPath(QString path);
    void setIntervalTime(int tempTime);
    int intervalTime() const;

    bool isShuffle() const;
    void setShuffle(const bool shuffle);

private:
    QString confPath();

protected:
    explicit SlideshowConfig(QObject *parent = nullptr);

private:
    SlideshowConfig(SlideshowConfig &) = delete;
    SlideshowConfig &operator=(SlideshowConfig &) = delete;

private:
    QScopedPointer<QSettings> m_settings;
};

#endif   // SLIDESHOWCONFIG_H
