// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIDEOSCREENSAVER_H
#define VIDEOSCREENSAVER_H

#include <QWidget>
#include <QUrl>

#ifdef ENABLE_DMR
#    include "player_engine.h"

class VideoView : public dmr::PlayerEngine
{
    Q_OBJECT
public:
    explicit VideoView(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
};
#else
class QVideoWidget;
class QMediaPlayer;
class QMediaPlaylist;
#endif

class VideoScreensaver : public QObject
{
    Q_OBJECT
public:
    explicit VideoScreensaver(QObject *parent = nullptr);
    ~VideoScreensaver();
    void init(QWidget *wid, int idx = 0);
    QWidget *view() const;
signals:

public slots:
protected:
    QList<QUrl> fileList() const;
    QList<QUrl> scanFile(const QString &dirPath) const;
    void playVideo(const QList<QUrl> &ret) const;

private:
#ifdef ENABLE_DMR
    VideoView *m_view = nullptr;
#else
    QVideoWidget *m_view = nullptr;
    QMediaPlayer *m_player = nullptr;
    QMediaPlaylist *m_playlist = nullptr;
#endif
    int m_duplicate = 0;
};

#endif   // VIDEOSCREENSAVER_H
