// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "videoscreensaver.h"

#ifdef ENABLE_DMR
#    include <player_engine.h>
#    include <player_widget.h>
#    include <QPaintEvent>
#else
#    include <QMediaContent>
#    include <QVideoWidget>
#    include <QMediaPlayer>
#    include <QMediaPlaylist>

#    include <malloc.h>
#endif

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QVariant>

#ifdef ENABLE_DMR
VideoView::VideoView(QWidget *parent)
    : dmr::PlayerEngine(parent)
{
}

void VideoView::closeEvent(QCloseEvent *event)
{
    stop();
    dmr::PlayerEngine::closeEvent(event);
    _Exit(0);
}
#endif

VideoScreensaver::VideoScreensaver(QObject *parent)
    : QObject(parent)
{
}

VideoScreensaver::~VideoScreensaver()
{
#ifndef ENABLE_DMR
    delete m_view;
    m_view = nullptr;

    delete m_playlist;
    m_playlist = nullptr;

    delete m_player;
    m_player = nullptr;
#endif
}

void VideoScreensaver::init(QWidget *wid, int idx)
{
    if (m_view)
        return;

    m_duplicate = idx;
#ifdef ENABLE_DMR
    m_view = new VideoView(wid);

    if (wid) {
        wid->layout()->addWidget(m_view);
    }

    m_view->setMute(true);
    m_view->getplaylist()->setPlayMode(dmr::PlaylistModel::ListLoop);
    m_view->setBackendProperty("ao", "no");   // 不解析音频
    m_view->setBackendProperty("color", QVariant::fromValue(QColor(Qt::black)));   // background color
#else
    m_view = new QVideoWidget();
    m_playlist = new QMediaPlaylist();
    m_player = new QMediaPlayer();
    m_playlist->setPlaybackMode(QMediaPlaylist::Loop);

    m_player->setPlaylist(m_playlist);
    m_player->setVideoOutput(m_view);
    m_player->setMuted(true);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [](QMediaPlayer::MediaStatus status) {
        // Force reclaiming memory
        if (QMediaPlayer::BufferedMedia == status)
            malloc_trim(0);
    });
#endif

    // black background.
    {
        auto pal = m_view->palette();
        pal.setColor(m_view->backgroundRole(), Qt::black);
        m_view->setPalette(pal);
    }

    playVideo(fileList());
}

QWidget *VideoScreensaver::view() const
{
    return m_view;
}

QList<QUrl> VideoScreensaver::fileList() const
{
    QList<QUrl> ret;
    auto paths = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
    if (!paths.isEmpty()) {
        // ~/.local/share/deepin/deepin-video-screensaver/resource
        QString path = paths.first() + "/resource";
        if (m_duplicate > 0)
            path += QString(".%0").arg(m_duplicate);
        qInfo() << "resource path" << path;

        ret = scanFile(path);
        if (!ret.isEmpty())
            return ret;
        qWarning() << "Warning: no mp4 file in " << path;
    } else {
        qWarning() << "error: no movie path";
    }

    // get default
    {
        QString defaultPath = QString("/usr/share/deepin-video-screensaver/resource");
        if (m_duplicate > 0)
            defaultPath += QString(".%0").arg(m_duplicate);
        qInfo() << "using defalult resource" << defaultPath;
        ret = scanFile(defaultPath);
    }

    return ret;
}

QList<QUrl> VideoScreensaver::scanFile(const QString &dirPath) const
{
    QList<QUrl> ret;

    QDir dir(dirPath);
#if 0   // scan the dir and get all mp4 file.
    static const QStringList validSuffix {QStringLiteral("*.mp4")};
    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
    QFileInfoList infoList = dir.entryInfoList(validSuffix, filters, QDir::Name);
    for (auto info : infoList)
        ret.append(QUrl::fromLocalFile(info.absoluteFilePath()));
#else   // just a specific file
    const QString fileName = "deepin-video-screensaver.mp4";
    auto filePath = dir.filePath(fileName);
    if (QFile::exists(filePath))
        ret.append(QUrl::fromLocalFile(filePath));
#endif
    return ret;
}

void VideoScreensaver::playVideo(const QList<QUrl> &ret) const
{
#ifdef ENABLE_DMR
    if (!ret.isEmpty()) {
        m_view->addPlayFiles(ret);
        m_view->playByName(ret.first());
    }
#else
    QList<QMediaContent> mediaList;
    for (auto media : ret)
        mediaList.append(media);
    m_playlist->addMedia(mediaList);
    m_player->play();
#endif
}
