// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowscreensaver.h"
#include "slideshowconfig.h"

#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QWindow>
#include <QScreen>
#include <QPainter>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QDebug>

SlideshowScreenSaver::SlideshowScreenSaver(bool subWindow, QWidget *parent)
    : QWidget(parent)
    , m_subWindow(subWindow)
{
    if (m_subWindow)
        setWindowFlag(Qt::WindowTransparentForInput, true);

    QString path = SlideshowConfig::instance()->slideshowPath();

    QDir dir(path);
    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
    QFileInfoList infoList = dir.entryInfoList(filters, QDir::Name);
    if (infoList.isEmpty())
        qWarning() << "Warning:no image file in " << path;

    static const QStringList validSuffix {QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("bmp"), QStringLiteral("png")};

    for (auto info : infoList) {
        if (validSuffix.contains(info.suffix())) {
            // 记录图片列表
            m_imagefiles.append(info.absoluteFilePath());

            if (m_pixmap.isNull()) {
                m_pixmap.reset(new QPixmap(info.absoluteFilePath()));
                m_currentImage = 0;
            }
        }
    }

    m_shuffle = SlideshowConfig::instance()->isShuffle();
    m_intervalTime = SlideshowConfig::instance()->intervalTime();

    m_timer.reset(new QTimer);
    connect(m_timer.get(), &QTimer::timeout, this, &SlideshowScreenSaver::onUpdateImage);

    m_timer->setInterval(m_intervalTime);
    m_timer->start();
}

SlideshowScreenSaver::~SlideshowScreenSaver()
{

}

void SlideshowScreenSaver::onUpdateImage()
{
    if (m_imagefiles.isEmpty())
        return;

    if (m_shuffle) {
        m_currentImage = int(QRandomGenerator::global()->generate() % uint(m_imagefiles.count()) - 1);
    } else {
        if (m_currentImage < m_imagefiles.count() - 1)
            m_currentImage++;
        else
            m_currentImage = 0;
    }

    m_pixmap.reset(new QPixmap(m_imagefiles.at(m_currentImage)));
    update();
}

void SlideshowScreenSaver::paintEvent(QPaintEvent *event)
{
    if (m_pixmap) {

        qreal scale = devicePixelRatioF();

        QSize trueSize(this->geometry().size());

        auto pix = m_pixmap->scaled(trueSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        if (pix.width() > trueSize.width() || pix.height() > trueSize.height()) {
            pix = pix.copy(QRect(static_cast<int>((pix.width() - trueSize.width()) / 2.0),
                                 static_cast<int>((pix.height() - trueSize.height()) / 2.0),
                                 trueSize.width(),
                                 trueSize.height()));
        }
        pix.setDevicePixelRatio(scale);

        QPainter pa(this);
        pa.drawPixmap(event->rect().topLeft(), pix, QRectF(QPointF(event->rect().topLeft()) * scale, QSizeF(event->rect().size()) * scale));
    }

    return QWidget::paintEvent(event);
}
